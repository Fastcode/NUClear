/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "NUClearNet.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <set>
#include <system_error>
#include <utility>
#include <vector>

#include <sstream>

#include "../util/network/resolve.hpp"
#include "../util/platform.hpp"
#include "Discovery.hpp"
#include "Fragmentation.hpp"
#include "Log.hpp"
#include "Reliability.hpp"
#include "wire_protocol.hpp"

namespace NUClear {
namespace network {

namespace {

    iovec make_iovec(void* base, std::size_t len) {
#ifdef _WIN32
        iovec iov{};
        iov.buf = reinterpret_cast<CHAR*>(base);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        iov.len = static_cast<ULONG>(len);
        return iov;
#else
        iovec iov{};
        iov.iov_base = base;
        iov.iov_len  = len;
        return iov;
#endif
    }

    /// Returns true for errors that indicate the socket itself is dead (interface down, fd invalid).
    /// These warrant closing and reopening the sockets rather than silently dropping the packet.
    bool is_fatal_socket_error(int err) {
#ifdef _WIN32
        return err == WSAENETDOWN || err == WSAENETRESET || err == WSAENETUNREACH || err == WSAENOTSOCK;
#else
        return err == ENETDOWN || err == ENETUNREACH || err == EBADF || err == ENOTSOCK
#    ifdef ENONET
               || err == ENONET  // Linux-specific: machine is not on the network
#    endif
            ;
#endif
    }

}  // namespace

    const std::chrono::milliseconds NUClearNet::ANNOUNCE_INTERVAL(500);  // NOLINT(cert-err58-cpp)

    void NUClearNet::set_log_level(LogLevel level) {
        network::set_log_level(level);
    }

    NUClearNet::NUClearNet()
        : discovery(std::make_unique<Discovery>(std::chrono::seconds(2)))
        , fragmentation(std::make_unique<Fragmentation>(1452, 64 * 1024 * 1024, std::chrono::seconds(2)))
        , reliability(std::make_unique<Reliability>()) {
        wire_discovery_callbacks();
    }

    NUClearNet::~NUClearNet() {
        try {
            shutdown();
        }
        catch (...) {  // NOLINT(bugprone-empty-catch)
            // Destructor must not throw
        }
    }

    void NUClearNet::wire_discovery_callbacks() {
        discovery->set_join_callback([this](const PeerInfo& peer) {
            routing.update_peer_subscriptions(peer.address, peer.subscriptions);
            if (join_callback) {
                join_callback(peer);
            }
        });

        discovery->set_leave_callback([this](const PeerInfo& peer) {
            routing.remove_peer(peer.address);
            reliability->remove_peer(peer.address);
            deduplicators.erase(peer.address);
            if (leave_callback) {
                leave_callback(peer);
            }
        });

        discovery->set_subscription_change_callback([this](const PeerInfo& peer) {
            routing.update_peer_subscriptions(peer.address, peer.subscriptions);
        });
    }

    void NUClearNet::reset(const NetworkConfig& new_config) {
        // Shut down existing connections
        shutdown();

        // Clear per-peer state that should not survive a reset
        deduplicators.clear();

        config    = new_config;
        node_name = new_config.name;

        // Update module configurations
        discovery     = std::make_unique<Discovery>(new_config.peer_timeout);
        fragmentation = std::make_unique<Fragmentation>(
            static_cast<uint16_t>(new_config.mtu - sizeof(DataPacket) + 1 - 40 - 8),  // MTU - headers
            new_config.max_assembly_size,
            new_config.peer_timeout);  // Assembly timeout matches peer timeout
        reliability = std::make_unique<Reliability>();

        wire_discovery_callbacks();

        // Open sockets with the new configuration
        open_sockets();

        if (should_log(LogLevel::Info)) {
            std::ostringstream msg;
            msg << "reset name=" << new_config.name << " announce=" << new_config.announce_address << ':'
                << new_config.announce_port << " mtu=" << new_config.mtu;
            log(LogLevel::Info, "net", msg.str());
        }
    }

    void NUClearNet::open_sockets() {
        data_fd.reset();
        announce_fd.reset();

        // Re-resolve announce target (the interface address may have changed)
        announce_target = util::network::resolve(config.announce_address, config.announce_port);

        // Determine bind address
        sock_t bind_addr{};
        if (config.bind_address.empty()) {
            bind_addr = announce_target;
            if (announce_target.sock.sa_family == AF_INET) {
                bind_addr.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
            }
            else if (announce_target.sock.sa_family == AF_INET6) {
                bind_addr.ipv6.sin6_addr = IN6ADDR_ANY_INIT;
            }
        }
        else {
            bind_addr = util::network::resolve(config.bind_address, config.announce_port);
        }

        // Open data socket (ephemeral port)
        {
            sock_t data_bind = bind_addr;
            if (data_bind.sock.sa_family == AF_INET) {
                data_bind.ipv4.sin_port = 0;
            }
            else {
                data_bind.ipv6.sin6_port = 0;
            }

            const fd_t fd = ::socket(data_bind.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
            if (fd < 0) {
                throw std::system_error(network_errno, std::system_category(), "Failed to create data socket");
            }

            int yes = 1;
            ::setsockopt(fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));

            if (::bind(fd, &data_bind.sock, data_bind.size()) != 0) {
                ::close(fd);
                throw std::system_error(network_errno, std::system_category(), "Failed to bind data socket");
            }

            // Set non-blocking so sends don't block when the kernel buffer is full
            // (unreliable sends should be fire-and-forget, not back-pressure the caller)
#ifdef _WIN32
            u_long non_blocking = 1;
            ioctl(fd, FIONBIO, &non_blocking);
#else
            const int flags = ::fcntl(fd, F_GETFL, 0);
            ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif

            sock_t bound_addr{};
            socklen_t bound_len = sizeof(bound_addr.storage);
            if (::getsockname(fd, &bound_addr.sock, &bound_len) != 0) {
                ::close(fd);
                throw std::system_error(network_errno, std::system_category(), "Failed to get data socket address");
            }
            own_data_address = bound_addr;

            data_fd.reset(fd);
        }

        // Open announce socket (known port)
        {
            const fd_t fd = ::socket(bind_addr.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
            if (fd < 0) {
                throw std::system_error(network_errno, std::system_category(), "Failed to create announce socket");
            }

            int yes = 1;
            ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
#ifdef SO_REUSEPORT
            ::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&yes), sizeof(yes));
#endif
            ::setsockopt(fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));

            if (::bind(fd, &bind_addr.sock, bind_addr.size()) != 0) {
                ::close(fd);
                throw std::system_error(network_errno, std::system_category(), "Failed to bind announce socket");
            }

            // Join multicast group if applicable
            const bool multicast = (announce_target.sock.sa_family == AF_INET
                                    && (ntohl(announce_target.ipv4.sin_addr.s_addr) & 0xF0000000U) == 0xE0000000U)
                                   || (announce_target.sock.sa_family == AF_INET6
                                       && announce_target.ipv6.sin6_addr.s6_addr[0] == 0xFF);
            if (multicast) {
                if (announce_target.sock.sa_family == AF_INET) {
                    ip_mreq mreq{};
                    mreq.imr_multiaddr = announce_target.ipv4.sin_addr;
                    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
                    if (::setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq)) < 0) {
                        ::close(fd);
                        throw std::system_error(network_errno, std::system_category(), "Failed to join multicast group");
                    }

                    // Allow multicast loopback for single-host development
                    int loop = 1;
                    ::setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<const char*>(&loop), sizeof(loop));
                }
                else {
                    ipv6_mreq mreq{};
                    mreq.ipv6mr_multiaddr = announce_target.ipv6.sin6_addr;
                    mreq.ipv6mr_interface = 0;
                    if (::setsockopt(fd, IPPROTO_IPV6, IPV6_JOIN_GROUP, reinterpret_cast<const char*>(&mreq), sizeof(mreq)) < 0) {
                        ::close(fd);
                        throw std::system_error(network_errno, std::system_category(), "Failed to join IPv6 multicast group");
                    }
                }
            }

            // Set non-blocking so read_socket drain loop works on Windows
            // (where MSG_DONTWAIT has no effect)
#ifdef _WIN32
            u_long non_blocking = 1;
            ioctl(fd, FIONBIO, &non_blocking);
#else
            const int flags = ::fcntl(fd, F_GETFL, 0);
            ::fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#endif

            announce_fd.reset(fd);
        }

        // Force an immediate announce on the new sockets
        last_announce = std::chrono::steady_clock::time_point{};
    }

    void NUClearNet::shutdown() {
        if (should_log(LogLevel::Info)) {
            log(LogLevel::Info, "net", "shutdown");
        }

        // Send leave packet to announce address if we have a data socket
        if (data_fd.valid()) {
            auto leave = Discovery::build_leave_packet();
            send_buf(data_fd, announce_target, leave.data(), leave.size());
        }
        own_data_address = {};
        data_fd.reset();
        announce_fd.reset();
    }

    void NUClearNet::process() {
        // Attempt to rebind if a previous send or receive indicated the sockets are dead.
        // This recovers from interface changes (e.g., switching WiFi networks).
        if (needs_rebind) {
            if (should_log(LogLevel::Info)) {
                log(LogLevel::Info, "net", "rebinding sockets after fatal socket error");
            }
            const auto retry_at = std::chrono::steady_clock::now() + ANNOUNCE_INTERVAL;

            // Evict all peers: fires leave callbacks which clean up routing and reliability state
            discovery->clear_peers();
            deduplicators.clear();

            // Discard stale fragmentation assemblies and reliability tracking from the old path
            fragmentation = std::make_unique<Fragmentation>(
                static_cast<uint16_t>(config.mtu - sizeof(DataPacket) + 1 - 40 - 8),
                config.max_assembly_size,
                config.peer_timeout);
            reliability = std::make_unique<Reliability>();

            try {
                open_sockets();
                needs_rebind = false;
            }
            catch (...) {
                // Network not available yet; will retry on the next process() call
            }

            if (!needs_rebind && socket_change_callback) {
                // Notify the caller so it can update IO event registrations for the new fds
                socket_change_callback();
            }

            // Always schedule a follow-up call so we retry if the rebind failed
            if (event_callback) {
                event_callback(retry_at);
            }
            return;
        }

        if (should_log(LogLevel::Trace)) {
            log(LogLevel::Trace, "net", "process begin");
        }

        auto now = std::chrono::steady_clock::now();

        // Send announce if interval has elapsed
        if (now - last_announce >= ANNOUNCE_INTERVAL) {
            announce();
            last_announce = now;
            if (should_log(LogLevel::Trace)) {
                log(LogLevel::Trace, "net", "announce sent");
            }
        }

        // Check for timed-out peers
        discovery->check_timeouts(now);

        // Check for retransmissions
        auto retransmissions = reliability->check_retransmissions(fragmentation->get_packet_mtu());
        if (should_log(LogLevel::Debug) && !retransmissions.empty()) {
            std::ostringstream msg;
            msg << "retransmit count=" << retransmissions.size();
            for (const auto& req : retransmissions) {
                msg << " peer=" << sock_str(req.target) << " packet_id=" << req.packet_id << " frag="
                    << static_cast<int>(req.packet_no);
            }
            log(LogLevel::Debug, "net", msg.str());
        }
        for (const auto& req : retransmissions) {
            // Build header on stack, scatter-write header + fragment data
            DataPacket header{};
            header.packet_id    = req.packet_id;
            header.packet_no    = req.packet_no;
            header.packet_count = req.packet_count;
            header.flags        = req.flags;
            header.hash         = req.hash;

            std::array<iovec, 2> iov{
                make_iovec(reinterpret_cast<void*>(&header), sizeof(DataPacket) - 1),  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                make_iovec(const_cast<void*>(static_cast<const void*>(req.data.data())), req.data.size()),  // NOLINT(cppcoreguidelines-pro-type-const-cast)
            };

            send_iov(data_fd, req.target, iov.data(), static_cast<int>(iov.size()));
        }

        // Clean up expired fragment assemblies
        fragmentation->cleanup_expired();

        // Read pending packets from both sockets
        if (announce_fd.valid()) {
            read_socket(announce_fd);
        }
        if (data_fd.valid()) {
            read_socket(data_fd);
        }

        // Schedule next event
        if (event_callback) {
            const auto next = now + ANNOUNCE_INTERVAL;
            if (should_log(LogLevel::Trace)) {
                const auto ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(next - now).count();
                log(LogLevel::Trace, "net", "schedule next event in " + std::to_string(ms) + "ms");
            }
            event_callback(next);
        }

        if (should_log(LogLevel::Trace)) {
            log(LogLevel::Trace, "net", "process end");
        }
    }

    void NUClearNet::send(uint64_t hash, const uint8_t* payload, std::size_t length, const std::string& target, bool reliable) {
        if (!data_fd.valid()) {
            return;
        }

        // Get a packet ID
        uint16_t packet_id = next_packet_id++;

        uint8_t flags = reliable ? RELIABLE : 0;

        // Compute fragment count
        const uint16_t packet_mtu   = fragmentation->get_packet_mtu();
        const uint16_t packet_count = Fragmentation::compute_fragment_count(length, packet_mtu);

        // Send all fragments of a message to a given destination
        auto send_fragments = [&](const sock_t& dest) {
            for (uint16_t i = 0; i < packet_count; ++i) {
                DataPacket header{};
                header.packet_id    = packet_id;
                header.packet_no    = i;
                header.packet_count = packet_count;
                header.flags        = flags;
                header.hash         = hash;

                const std::size_t offset   = static_cast<std::size_t>(i) * packet_mtu;
                const std::size_t frag_len = std::min(static_cast<std::size_t>(packet_mtu), length - offset);

                std::array<iovec, 2> iov{
                    make_iovec(reinterpret_cast<void*>(&header), sizeof(DataPacket) - 1),  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                    make_iovec(const_cast<void*>(static_cast<const void*>(payload + offset)), frag_len),  // NOLINT(cppcoreguidelines-pro-type-const-cast)
                };

                send_iov(data_fd, dest, iov.data(), static_cast<int>(iov.size()));
            }
        };

        // Unreliable broadcast: send once to the multicast/broadcast group
        // All connected peers receive it on their announce socket — receivers filter by subscription
        if (target.empty() && !reliable) {
            if (should_log(LogLevel::Debug)) {
                std::ostringstream msg;
                msg << "send broadcast hash=" << hash_hex(hash) << " len=" << length
                    << " frags=" << packet_count << " reliable=" << reliable;
                log(LogLevel::Debug, "net", msg.str());
            }
            send_fragments(announce_target);
            return;
        }

        // Targeted or reliable sends: unicast to each matching peer
        auto peers = discovery->get_peers();
        std::vector<sock_t> targets;

        if (target.empty()) {
            // Reliable broadcast: send to all subscribing peers individually (for ACK tracking)
            for (const auto& peer : peers) {
                const auto& addr = peer.first;
                if (routing.should_send(addr, hash)) {
                    targets.push_back(addr);
                }
            }
        }
        else {
            // Send to specific named peer(s)
            for (const auto& peer : peers) {
                const auto& addr = peer.first;
                const auto& info = peer.second;
                if (info.name == target && routing.should_send(addr, hash)) {
                    targets.push_back(addr);
                }
            }
        }

        if (targets.empty()) {
            if (should_log(LogLevel::Debug)) {
                std::ostringstream msg;
                msg << "send dropped (no targets) hash=" << hash_hex(hash) << " target="
                    << (target.empty() ? "<broadcast>" : target);
                log(LogLevel::Debug, "net", msg.str());
            }
            return;
        }

        if (should_log(LogLevel::Debug)) {
            std::ostringstream msg;
            msg << "send hash=" << hash_hex(hash) << " len=" << length << " frags=" << packet_count
                << " reliable=" << reliable << " targets=" << targets.size();
            if (!target.empty()) {
                msg << " name=" << target;
            }
            log(LogLevel::Debug, "net", msg.str());
        }

        // Send each fragment to each target
        for (const auto& tgt : targets) {
            send_fragments(tgt);

            // If reliable, track for retransmission (single copy stored internally)
            if (reliable) {
                reliability->track_packet(tgt, packet_id, packet_count, hash, flags, payload, length);
            }
        }
    }

    void NUClearNet::set_subscriptions(const std::set<uint64_t>& subscriptions) {
        routing.set_local_subscriptions(subscriptions);
        if (should_log(LogLevel::Debug)) {
            std::ostringstream msg;
            msg << "set_subscriptions count=" << subscriptions.size();
            for (const auto& h : subscriptions) {
                msg << ' ' << hash_hex(h);
            }
            log(LogLevel::Debug, "net", msg.str());
        }
        // Immediately announce so peers learn the updated subscription list without waiting
        // for the next periodic interval
        announce();
        last_announce = std::chrono::steady_clock::now();
    }

    void NUClearNet::add_subscription(uint64_t hash) {
        routing.add_local_subscription(hash);
        if (should_log(LogLevel::Debug)) {
            log(LogLevel::Debug, "net", "add_subscription " + hash_hex(hash));
        }
        // Immediately announce so peers learn the new subscription without waiting for
        // the next periodic interval
        announce();
        last_announce = std::chrono::steady_clock::now();
    }

    void NUClearNet::set_packet_callback(PacketCallback cb) {
        packet_callback = std::move(cb);
    }

    void NUClearNet::set_join_callback(JoinCallback cb) {
        join_callback = std::move(cb);
    }

    void NUClearNet::set_leave_callback(LeaveCallback cb) {
        leave_callback = std::move(cb);
    }

    void NUClearNet::set_event_callback(EventCallback cb) {
        event_callback = std::move(cb);
    }

    void NUClearNet::set_socket_change_callback(SocketChangeCallback cb) {
        socket_change_callback = std::move(cb);
    }

    std::vector<fd_t> NUClearNet::listen_fds() const {
        std::vector<fd_t> fds;
        if (data_fd.valid()) {
            fds.push_back(data_fd.get());
        }
        if (announce_fd.valid()) {
            fds.push_back(announce_fd.get());
        }
        return fds;
    }

    bool NUClearNet::is_own_data_endpoint(const sock_t& source) const {
        if (own_data_address.sock.sa_family == AF_UNSPEC) {
            return false;
        }
        if (source.sock.sa_family != own_data_address.sock.sa_family) {
            return false;
        }
        if (source.sock.sa_family == AF_INET) {
            return source.ipv4.sin_port == own_data_address.ipv4.sin_port;
        }
        if (source.sock.sa_family == AF_INET6) {
            return source.ipv6.sin6_port == own_data_address.ipv6.sin6_port;
        }
        return false;
    }

    void NUClearNet::announce() {
        if (!data_fd.valid()) {
            return;
        }

        auto subs = routing.get_local_subscriptions();
        auto packet = Discovery::build_announce_packet(node_name, subs);

        // Send announce from data socket to announce target (NAT-friendly)
        send_buf(data_fd, announce_target, packet.data(), packet.size());
    }

    void NUClearNet::read_socket(fd_t fd) {
        // Stack buffer — 65535 is the maximum UDP datagram size
        alignas(8) std::array<uint8_t, 65535> buffer{};
        sock_t source{};

        // Drain all pending datagrams from the socket
        // The data socket is non-blocking, so recvfrom returns -1/EWOULDBLOCK when empty
        // For the announce socket, MSG_DONTWAIT provides the same behavior on POSIX
        socklen_t source_len = 0;
        auto recv = [&]() {
            source     = {};  // Clear stale bytes so different address families don't corrupt the assembly key
            source_len = sizeof(source.storage);
            return ::recvfrom(fd,
                              reinterpret_cast<char*>(buffer.data()),
                              static_cast<int>(buffer.size()),
                              MSG_DONTWAIT,
                              &source.sock,
                              &source_len);
        };

        std::size_t datagrams = 0;
        ssize_t received = recv();
        while (received > 0) {
            ++datagrams;
            process_packet(source, buffer.data(), static_cast<std::size_t>(received));
            received = recv();
        }

        if (should_log(LogLevel::Trace) && datagrams > 0) {
            log(LogLevel::Trace, "net", "read_socket fd=" + std::to_string(fd) + " datagrams=" + std::to_string(datagrams));
        }

        // A negative return that is not a normal non-blocking condition (EAGAIN/EWOULDBLOCK)
        // means the socket itself has failed — schedule a rebind
        if (received < 0 && is_fatal_socket_error(network_errno)) {
            needs_rebind = true;
        }
    }

    void NUClearNet::process_packet(const sock_t& source, const uint8_t* data, std::size_t length) {
        if (!validate_header(data, length)) {
            if (should_log(LogLevel::Warn)) {
                log(LogLevel::Warn, "net",
                    "invalid packet header from " + sock_str(source) + " len=" + std::to_string(length));
            }
            return;
        }

        const auto* header = reinterpret_cast<const PacketHeader*>(data);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

        // Touch the peer to reset timeout
        discovery->touch_peer(source);

        switch (header->type) {
            case ANNOUNCE: process_announce_packet(source, data, length); return;
            case LEAVE: process_leave_packet(source); return;
            case CONNECT: process_connect_packet(source, data, length); return;
            case DATA: process_data_packet(source, data, length); return;
            case ACK: process_ack_packet(source, data, length); return;
            default: return;
        }
    }

    void NUClearNet::process_announce_packet(const sock_t& source, const uint8_t* data, std::size_t length) {
        // Ignore our own announces (multicast/broadcast loopback) without blocking other nodes on 127.0.0.1
        if (is_own_data_endpoint(source)) {
            if (should_log(LogLevel::Debug)) {
                log(LogLevel::Debug, "net", "ignoring self announce from " + sock_str(source));
            }
            return;
        }

        auto announce_result = discovery->process_announce(source, data, length);

        if (announce_result.is_new) {
            // Force an immediate announce to the multicast/broadcast group
            // so the new peer hears us on the announce channel (confirms our_d→their_a)
            announce();
            last_announce = std::chrono::steady_clock::now();
        }

        // Send CONNECT packet if the handshake needs it (initial SYN or retransmit)
        if (announce_result.response_flags != 0) {
            auto pkt = Discovery::build_connect_packet(announce_result.response_flags);
            send_buf(data_fd, source, pkt.data(), pkt.size());

            // Mark SYN_SENT if we're sending a SYN (only advances from IDLE)
            if ((announce_result.response_flags & SYN) != 0) {
                discovery->mark_syn_sent(source);
            }
        }
    }

    void NUClearNet::process_leave_packet(const sock_t& source) {
        discovery->process_leave(source);
    }

    void NUClearNet::process_connect_packet(const sock_t& source, const uint8_t* data, std::size_t length) {
        if (length < sizeof(ConnectPacket)) {
            if (should_log(LogLevel::Warn)) {
                log(LogLevel::Warn, "net",
                    "short CONNECT from " + sock_str(source) + " len=" + std::to_string(length));
            }
            return;
        }
        const auto* pkt = reinterpret_cast<const ConnectPacket*>(data);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        auto result     = discovery->process_connect(source, pkt->flags);

        // Send response if the state machine requires one
        if (result.response_flags != 0) {
            auto response = Discovery::build_connect_packet(result.response_flags);
            send_buf(data_fd, source, response.data(), response.size());
        }
    }

    void NUClearNet::process_data_packet(const sock_t& source, const uint8_t* data, std::size_t length) {
        // Only accept data from connected peers
        if (!discovery->is_connected(source)) {
            if (should_log(LogLevel::Debug)) {
                log(LogLevel::Debug, "net", "DATA from unconnected " + sock_str(source));
            }
            return;
        }

        if (length < sizeof(DataPacket)) {
            if (should_log(LogLevel::Warn)) {
                log(LogLevel::Warn, "net",
                    "short DATA from " + sock_str(source) + " len=" + std::to_string(length));
            }
            return;
        }

        const auto* pkt = reinterpret_cast<const DataPacket*>(data);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

        if (pkt->packet_count == 0 || pkt->packet_no >= pkt->packet_count) {
            if (should_log(LogLevel::Warn)) {
                log(LogLevel::Warn, "net",
                    "invalid DATA fragment indices from " + sock_str(source) + " id=" + std::to_string(pkt->packet_id)
                        + " no=" + std::to_string(pkt->packet_no) + " count=" + std::to_string(pkt->packet_count));
            }
            return;
        }

        // Drop messages we are not subscribed to (relevant for multicast broadcast data)
        if (!routing.is_locally_subscribed(pkt->hash)) {
            return;
        }

        // Check for duplicates (at the packet group level)
        auto& dedup = deduplicators[source];

        if (dedup.is_duplicate(pkt->packet_id)) {
            // Already processed this packet group — send ACK if reliable
            if ((pkt->flags & RELIABLE) != 0) {
                const std::vector<bool> all_received(pkt->packet_count, true);
                auto ack = Reliability::build_ack_packet(pkt->packet_id, pkt->packet_count, all_received);
                send_buf(data_fd, source, ack.data(), ack.size());
            }
            return;
        }

        // Extract fragment data
        const uint8_t* frag_data        = data + sizeof(DataPacket) - 1;
        const std::size_t frag_length   = length - (sizeof(DataPacket) - 1);

        // Use a hash of the full source address as the source key for fragmentation
        // This ensures IPv6 addresses are properly distinguished
        uint64_t source_key         = 0;
        const auto* bytes = reinterpret_cast<const uint8_t*>(&source.storage);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        for (std::size_t i = 0; i < sizeof(source.storage); ++i) {
            source_key ^= static_cast<uint64_t>(bytes[i]) << ((i % 8) * 8);
        }

        // Submit to fragmentation
        Fragmentation::AssembledPacket assembled;
        const bool has_assembled = fragmentation->submit_fragment(source_key,
                                                            pkt->packet_id,
                                                            pkt->packet_no,
                                                            pkt->packet_count,
                                                            pkt->hash,
                                                            pkt->flags,
                                                            frag_data,
                                                            frag_length,
                                                            assembled);

        // Send ACK for reliable packets
        if ((pkt->flags & RELIABLE) != 0) {
            std::vector<bool> received(pkt->packet_count, false);
            received[pkt->packet_no] = true;
            auto ack = Reliability::build_ack_packet(pkt->packet_id, pkt->packet_count, received);
            send_buf(data_fd, source, ack.data(), ack.size());
        }

        // If we have a complete message, deliver it
        if (has_assembled) {
            dedup.add_packet(assembled.packet_id);

            if (packet_callback) {
                // Look up peer name
                std::string peer_name;
                PeerInfo peer;
                if (discovery->get_peer(source, peer)) {
                    peer_name = peer.name;
                }

                const bool reliable_flag = (assembled.flags & RELIABLE) != 0;
                if (should_log(LogLevel::Debug)) {
                    std::ostringstream msg;
                    msg << "deliver hash=" << hash_hex(assembled.hash) << " peer=" << peer_name << " ("
                        << sock_str(source) << ") len=" << assembled.payload.size()
                        << " reliable=" << reliable_flag;
                    log(LogLevel::Debug, "net", msg.str());
                }
                packet_callback(source, peer_name, assembled.hash, reliable_flag, std::move(assembled.payload));
            }

            // Send full ACK for reliable
            if ((assembled.flags & RELIABLE) != 0) {
                const std::vector<bool> all_received(pkt->packet_count, true);
                auto ack = Reliability::build_ack_packet(pkt->packet_id, pkt->packet_count, all_received);
                send_buf(data_fd, source, ack.data(), ack.size());
            }
        }
    }

    void NUClearNet::process_ack_packet(const sock_t& source, const uint8_t* data, std::size_t length) {
        // Only accept ACKs from connected peers
        if (!discovery->is_connected(source)) {
            return;
        }

        if (length < sizeof(ACKPacket)) {
            if (should_log(LogLevel::Warn)) {
                log(LogLevel::Warn, "net",
                    "short ACK from " + sock_str(source) + " len=" + std::to_string(length));
            }
            return;
        }
        const auto* pkt = reinterpret_cast<const ACKPacket*>(data);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
        const uint8_t* bitset       = data + sizeof(ACKPacket) - 1;
        const std::size_t bitset_size = length - (sizeof(ACKPacket) - 1);
        reliability->process_ack(source, pkt->packet_id, pkt->packet_count, bitset, bitset_size);
    }

    void NUClearNet::send_iov(fd_t fd, const sock_t& target, const iovec* iov, int iovcnt) {  // NOLINT(readability-convert-member-functions-to-static)
        if (fd == INVALID_SOCKET) {
            return;
        }

        msghdr msg{};
        msg.msg_name       = const_cast<sockaddr*>(&target.sock);   // NOLINT(cppcoreguidelines-pro-type-const-cast)
        msg.msg_namelen    = target.size();
        msg.msg_iov        = const_cast<iovec*>(iov);               // NOLINT(cppcoreguidelines-pro-type-const-cast)
        msg.msg_iovlen     = static_cast<decltype(msg.msg_iovlen)>(iovcnt);
        msg.msg_control    = nullptr;
        msg.msg_controllen = 0;
#ifndef _WIN32
        msg.msg_flags = 0;
#endif

#ifdef _WIN32
        if (NUClear::sendmsg(fd, &msg, 0) < 0 && is_fatal_socket_error(network_errno)) {
            needs_rebind = true;
        }
#else
        if (::sendmsg(fd, &msg, 0) < 0 && is_fatal_socket_error(network_errno)) {
            needs_rebind = true;
        }
#endif
    }

}  // namespace network
}  // namespace NUClear
