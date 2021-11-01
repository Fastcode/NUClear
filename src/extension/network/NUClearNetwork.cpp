/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include "NUClearNetwork.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <set>
#include <utility>

#include "../../util/network/get_interfaces.hpp"
#include "../../util/platform.hpp"

namespace NUClear {
namespace extension {
    namespace network {

        socklen_t socket_size(const util::network::sock_t& s) {
            switch (s.sock.sa_family) {
                case AF_INET: return sizeof(sockaddr_in);
                case AF_INET6: return sizeof(sockaddr_in6);
                default: throw std::runtime_error("unhandled socket address family");
            }
        }

        NUClearNetwork::PacketQueue::PacketTarget::PacketTarget(std::weak_ptr<NetworkTarget> target,
                                                                std::vector<uint8_t> acked)
            : target(std::move(target)), acked(std::move(acked)), last_send(std::chrono::steady_clock::now()) {}

        NUClearNetwork::PacketQueue::PacketQueue() = default;

        NUClearNetwork::NUClearNetwork()
            : data_fd(INVALID_SOCKET)
            , announce_fd(INVALID_SOCKET)
            , packet_data_mtu(1000)
            , packet_id_source(0)
            , last_announce(std::chrono::seconds(0))
            , next_event(std::chrono::seconds(0)) {}


        NUClearNetwork::~NUClearNetwork() {
            shutdown();
        }

        void NUClearNetwork::set_packet_callback(
            std::function<void(const NetworkTarget&, const uint64_t&, const bool&, std::vector<char>&&)> f) {
            packet_callback = std::move(f);
        }


        void NUClearNetwork::set_join_callback(std::function<void(const NetworkTarget&)> f) {
            join_callback = std::move(f);
        }


        void NUClearNetwork::set_leave_callback(std::function<void(const NetworkTarget&)> f) {
            leave_callback = std::move(f);
        }

        void NUClearNetwork::set_next_event_callback(std::function<void(std::chrono::steady_clock::time_point)> f) {
            next_event_callback = std::move(f);
        }

        std::array<uint16_t, 9> NUClearNetwork::udp_key(const sock_t& address) {

            // Get our keys for our maps, it will be the ip and then port
            std::array<uint16_t, 9> key;
            key.fill(0);

            switch (address.sock.sa_family) {
                case AF_INET:
                    // The first chars are 0 (ipv6) and after that is our address and then port
                    std::memcpy(&key[6], &address.ipv4.sin_addr, sizeof(address.ipv4.sin_addr));
                    key[8] = address.ipv4.sin_port;
                    break;

                case AF_INET6:
                    // IPv6 address then port
                    std::memcpy(key.data(), &address.ipv6.sin6_addr, sizeof(address.ipv6.sin6_addr));
                    key[8] = address.ipv6.sin6_port;
                    break;
            }

            return key;
        }


        void NUClearNetwork::remove_target(const std::shared_ptr<NetworkTarget>& target) {

            // Erase udp
            auto key = udp_key(target->target);
            if (udp_target.find(key) != udp_target.end()) { udp_target.erase(udp_target.find(key)); }

            // Erase name
            auto range = name_target.equal_range(target->name);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == target) {
                    name_target.erase(it);
                    break;
                }
            }

            // Erase target
            auto t = std::find(targets.begin(), targets.end(), target);
            if (t != targets.end()) { targets.erase(t); }
        }


        void NUClearNetwork::open_data(const sock_t& announce_target) {

            // Create the "join any" address for this address family
            sock_t address = announce_target;

            // IPv4
            if (address.sock.sa_family == AF_INET) {
                address.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
                address.ipv4.sin_port        = 0;
            }
            // IPv6
            else if (address.sock.sa_family == AF_INET6) {
                address.ipv6.sin6_addr = IN6ADDR_ANY_INIT;
                address.ipv6.sin6_port = 0;
            }

            // Open a socket with the same family as our announce target
            data_fd = ::socket(address.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
            if (data_fd < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to open the UDP socket");
            }

            // If we are a broadcast address we need to state we are explicitly before binding
            int yes = 1;
            if (::setsockopt(data_fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to set broadcast on the socket");
            }

            // Bind to the address, and if we fail throw an error
            if (::bind(data_fd, &address.sock, socket_size(address)) != 0) {
                throw std::system_error(
                    network_errno, std::system_category(), "Unable to bind the UDP socket to the port");
            }
        }


        void NUClearNetwork::open_announce(const sock_t& announce_target) {

            // Work out what type of announce target we are using
            sock_t address = announce_target;

            // Work out what type of announce we are doing as it will influence how we make the socket
            bool multicast =
                (address.sock.sa_family == AF_INET && (ntohl(address.ipv4.sin_addr.s_addr) & 0xF0000000) == 0xE0000000)
                || (address.sock.sa_family == AF_INET6 && address.ipv6.sin6_addr.s6_addr[0] == 0xFF
                    && address.ipv6.sin6_addr.s6_addr[1] == 0x00);

            // Swap our address so the rest of the information is anys
            if (address.sock.sa_family == AF_INET) { address.ipv4.sin_addr.s_addr = htonl(INADDR_ANY); }
            else if (address.sock.sa_family == AF_INET6) {
                address.ipv6.sin6_addr = IN6ADDR_ANY_INIT;
            }

            // Make our socket
            announce_fd = ::socket(address.sock.sa_family, SOCK_DGRAM, IPPROTO_UDP);
            if (announce_fd < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to open the UDP socket");
            }

            // Set that we reuse the address so more than one application can bind (this applies for unicast as well)
            int yes = 1;
            if (::setsockopt(announce_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to reuse address on the socket");
            }

// If SO_REUSEPORT is available set it too
#ifdef SO_REUSEPORT
            if (::setsockopt(announce_fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to reuse port on the socket");
            }
#endif

            // We enable SO_BROADCAST since sometimes we need to send broadcast packets
            if (::setsockopt(announce_fd, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&yes), sizeof(yes)) < 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to set broadcast on the socket");
            }

            // Bind to the address
            if (::bind(announce_fd, &address.sock, socket_size(address)) != 0) {
                throw std::system_error(network_errno, std::system_category(), "Unable to bind the UDP socket");
            }

            // If we have a multicast address, then we need to join the multicast groups
            if (multicast) {
                // Our multicast join request will depend on protocol version
                if (announce_target.sock.sa_family == AF_INET) {

                    // Set the multicast address we are listening on
                    ip_mreq mreq;
                    mreq.imr_multiaddr = announce_target.ipv4.sin_addr;

                    int connected_count    = 0;
                    int last_network_errno = 0;

                    // Join the multicast group on all the interfaces that support it
                    for (auto& iface : util::network::get_interfaces()) {

                        if (iface.ip.sock.sa_family == AF_INET && iface.flags.multicast) {

                            // Set our interface address
                            mreq.imr_interface = iface.ip.ipv4.sin_addr;

                            // Join our multicast group
                            int status = ::setsockopt(announce_fd,
                                                      IPPROTO_IP,
                                                      IP_ADD_MEMBERSHIP,
                                                      reinterpret_cast<char*>(&mreq),
                                                      sizeof(ip_mreq));

                            if (status < 0) { last_network_errno = network_errno; }
                            else {
                                connected_count++;
                            }
                        }
                    }

                    if (connected_count == 0) {
                        throw std::system_error(last_network_errno,
                                                std::system_category(),
                                                "There was an error while attempting to join the multicast group");
                    }
                }
                else if (announce_target.sock.sa_family == AF_INET6) {

                    // Set the multicast address we are listening on
                    ipv6_mreq mreq;
                    mreq.ipv6mr_multiaddr = announce_target.ipv6.sin6_addr;

                    std::set<unsigned int> added_interfaces;

                    // Join the multicast group on all the interfaces that support it
                    for (auto& iface : util::network::get_interfaces()) {

                        if (iface.ip.sock.sa_family == AF_INET6 && iface.flags.multicast) {

                            // Get the interface for this
                            mreq.ipv6mr_interface = if_nametoindex(iface.name.c_str());

                            // Only add each interface index once
                            if (added_interfaces.count(mreq.ipv6mr_interface) == 0) {
                                added_interfaces.insert(mreq.ipv6mr_interface);

                                // Join our multicast group
                                if (::setsockopt(announce_fd,
                                                 IPPROTO_IPV6,
                                                 IPV6_JOIN_GROUP,
                                                 reinterpret_cast<char*>(&mreq),
                                                 sizeof(ipv6_mreq))
                                    < 0) {
                                    throw std::system_error(
                                        network_errno,
                                        std::system_category(),
                                        "There was an error while attempting to join the multicast group");
                                }
                            }
                        }
                    }
                }
            }
        }


        void NUClearNetwork::shutdown() {

            // If we have an fd, send a shutdown message
            if (data_fd > 0) {
                // Make a leave packet from our announce packet
                LeavePacket packet;

                auto announce_targets = name_target.equal_range("");
                for (auto it = announce_targets.first; it != announce_targets.second; ++it) {

                    // Send the packet
                    sendto(data_fd,
                           reinterpret_cast<const char*>(&packet),
                           sizeof(packet),
                           0,
                           &it->second->target.sock,
                           socket_size(it->second->target));
                }
            }

            // Close our existing FDs if they exist
            if (data_fd > 0) {
                close(data_fd);
                data_fd = INVALID_SOCKET;
            }
            if (announce_fd > 0) {
                close(announce_fd);
                announce_fd = INVALID_SOCKET;
            }
        }


        void NUClearNetwork::reset(const std::string& name,
                                   const std::string& address,
                                   in_port_t port,
                                   uint16_t network_mtu) {

            // Close our existing FDs if they exist
            shutdown();

            // Lock all mutexes
            std::lock_guard<std::mutex> target_lock(target_mutex);
            std::lock_guard<std::mutex> send_lock(send_queue_mutex);

            // Clear all our data structures
            send_queue.clear();
            name_target.clear();
            targets.clear();
            udp_target.clear();

            // Setup some hints for what our address is
            addrinfo hints;
            memset(&hints, 0, sizeof hints);  // make sure the struct is empty
            hints.ai_family   = AF_UNSPEC;    // don't care about IPv4 or IPv6
            hints.ai_socktype = SOCK_DGRAM;   // using udp datagrams

            // Get our info on this address
            addrinfo* servinfo = nullptr;
            ::getaddrinfo(address.c_str(), std::to_string(port).c_str(), &hints, &servinfo);

            // Check if we have any addresses to work with
            if (servinfo == nullptr) {
                throw std::runtime_error(std::string("The multicast address provided (") + address + ") was invalid");
            }

            // Clear our struct
            sock_t announce_target;
            std::memset(&announce_target, 0, sizeof(announce_target));

            // The list is actually a linked list of valid addresses
            // The address we choose is in the following priority, IPv4, IPv6, Other
            for (addrinfo* p = servinfo; p != nullptr; p = p->ai_next) {

                // If we find an IPv4 address, prefer that
                if (servinfo->ai_family == AF_INET) {

                    // Clear and set our struct
                    std::memset(&announce_target, 0, sizeof(announce_target));
                    std::memcpy(&announce_target, servinfo->ai_addr, servinfo->ai_addrlen);

                    // We prefer IPv4 so use it and stop looking
                    break;
                }
                // If we find an IPv6 address we set that for now
                else if (servinfo->ai_family == AF_INET6) {

                    // Clear and set our struct
                    std::memset(&announce_target, 0, sizeof(announce_target));
                    std::memcpy(&announce_target, servinfo->ai_addr, servinfo->ai_addrlen);
                }
            }

            // Clean up our memory
            ::freeaddrinfo(servinfo);

            // If we couldn't find a useable address die
            if (announce_target.sock.sa_family != AF_INET && announce_target.sock.sa_family != AF_INET6) {
                throw std::runtime_error(std::string("The network address provided (") + address
                                         + ") was not a valid multicast address");
            }

            // Add the target for our multicast packets
            auto all_target = std::make_shared<NetworkTarget>("", announce_target);
            targets.push_front(all_target);
            name_target.insert(std::make_pair("", all_target));
            udp_target.insert(std::make_pair(udp_key(announce_target), all_target));

            // Work out our MTU for udp packets
            packet_data_mtu = network_mtu;              // Start with the total mtu
            packet_data_mtu -= sizeof(DataPacket) - 1;  // Now remove data packet header size
            // IPv6 headers are always 40 bytes, and IPv4 can be 20-60 but if we assume 40 for all cases it should
            // be safe enough
            packet_data_mtu -= 40;  // Remove size of an IPv4 header or IPv6 header
            packet_data_mtu -= 8;   // Size of a UDP packet header

            // Build our announce packet
            announce_packet.resize(sizeof(AnnouncePacket) + name.size(), 0);
            AnnouncePacket& pkt = *reinterpret_cast<AnnouncePacket*>(announce_packet.data());
            pkt                 = AnnouncePacket();
            std::memcpy(&pkt.name, name.c_str(), name.size());

            // Open our data socket and then our multicast one

            open_data(announce_target);
            open_announce(announce_target);
        }


        std::pair<util::network::sock_t, std::vector<char>> NUClearNetwork::read_socket(fd_t fd) {

            // Allocate a vector that can hold a datagram
            std::vector<char> payload(1500);
            iovec iov;
            iov.iov_base = payload.data();
            iov.iov_len  = static_cast<decltype(iov.iov_len)>(payload.size());

            // Who we are receiving from
            sock_t from;
            std::memset(&from, 0, sizeof(from));

            // Setup our message header to receive
            msghdr mh;
            memset(&mh, 0, sizeof(msghdr));
            mh.msg_name    = &from.sock;
            mh.msg_namelen = sizeof(from);
            mh.msg_iov     = &iov;
            mh.msg_iovlen  = 1;

            // Now read the data for real
            ssize_t received = recvmsg(fd, &mh, 0);
            payload.resize(received);

            return std::make_pair(from, std::move(payload));
        }


        void NUClearNetwork::process() {

            // Record the time
            auto now = std::chrono::steady_clock::now();

            // Check if we should announce now
            if (now - last_announce > std::chrono::milliseconds(500)) {
                last_announce = now;
                announce();

                // Update our event timer
                auto next_announce = now + std::chrono::milliseconds(500);
                if (next_announce > next_event) {
                    next_event = next_announce;

                    // Let the system know when we need attention again
                    next_event_callback(next_event);
                }
            }

            // We need to make this list outside mutex scope in case the callback needs the mutex
            std::vector<std::shared_ptr<NetworkTarget>> leavers;

            // Check if any of our existing connections have timed out
            /* Mutex Scope */ {
                std::lock_guard<std::mutex> lock(target_mutex);

                // Always skip the first element since it's the "all" target
                for (auto it = std::next(targets.begin(), 1); it != targets.end();) {

                    auto ptr = *it;
                    ++it;

                    if (now - ptr->last_update > std::chrono::seconds(2)) {

                        // Remove this, it timed out
                        leavers.push_back(ptr);
                        remove_target(ptr);
                    }
                }
            }

            // Run the callback for anyone that left
            for (auto& l : leavers) {
                leave_callback(*l);
            }

            // Check if we have packets to resend and if so resend
            if (!send_queue.empty()) { retransmit(); }

            // Used for storing how many bytes are available on a socket
            unsigned long count = 0;

            // Read packets from the multicast socket while there is data available
            ioctl(announce_fd, FIONREAD, &(count = 0));
            while (count > 0) {
                auto packet = read_socket(announce_fd);
                process_packet(packet.first, std::move(packet.second));
                ioctl(announce_fd, FIONREAD, &(count = 0));
            }

            // Check if we have a packet available on the data socket
            ioctl(data_fd, FIONREAD, &(count = 0));
            while (count > 0) {
                auto packet = read_socket(data_fd);
                process_packet(packet.first, std::move(packet.second));
                ioctl(data_fd, FIONREAD, &(count = 0));
            }
        }

        void NUClearNetwork::retransmit() {

            // Locking send_queue_mutex second after target_mutex
            std::lock_guard<std::mutex> target_lock(target_mutex);
            std::lock_guard<std::mutex> send_lock(send_queue_mutex);

            for (auto qit = send_queue.begin(); qit != send_queue.end();) {
                for (auto it = qit->second.targets.begin(); it != qit->second.targets.end();) {

                    // Get the pointer to our target
                    auto ptr = it->target.lock();

                    // If our pointer is valid (they haven't disconnected)
                    if (ptr) {

                        auto now     = std::chrono::steady_clock::now();
                        auto timeout = it->last_send + ptr->round_trip_time;

                        // Check if we should have expected an ack by now for some packets
                        if (timeout < now) {

                            // We last sent now
                            it->last_send = now;

                            // The next time we should check for a timeout
                            auto next_timeout = now + ptr->round_trip_time;
                            if (next_timeout < next_event) {
                                next_event = next_timeout;
                                next_event_callback(next_event);
                            }

                            // Work out which packets to resend and resend them
                            for (uint16_t i = 0; i < qit->second.header.packet_count; ++i) {
                                if ((it->acked[i / 8] & uint8_t(1 << (i % 8))) == 0) {
                                    send_packet(ptr->target, qit->second.header, i, qit->second.payload, true);
                                }
                            }
                        }

                        ++it;
                    }
                    // Remove them from the list
                    else {
                        it = qit->second.targets.erase(it);
                    }
                }

                if (qit->second.targets.empty()) { qit = send_queue.erase(qit); }
                else {
                    ++qit;
                }
            }
        }


        void NUClearNetwork::announce() {

            // Get all our targets that are global targets
            auto announce_targets = name_target.equal_range("");
            for (auto it = announce_targets.first; it != announce_targets.second; ++it) {

                // Send the packet
                if (::sendto(data_fd,
                             announce_packet.data(),
                             static_cast<socklen_t>(announce_packet.size()),
                             0,
                             &it->second->target.sock,
                             socket_size(it->second->target))
                    < 0) {
                    throw std::system_error(
                        network_errno, std::system_category(), "Network error when sending the announce packet");
                }
            }
        }


        void NUClearNetwork::process_packet(const sock_t& address, std::vector<char>&& payload) {

            // First validate this is a NUClear network packet we can read (a version 2 NUClear packet)
            if (payload.size() >= sizeof(PacketHeader) && payload[0] == '\xE2' && payload[1] == '\x98'
                && payload[2] == '\xA2' && payload[3] == 0x02) {

                // This is a real packet! get our header information
                const PacketHeader& header = *reinterpret_cast<const PacketHeader*>(payload.data());

                // Get the map key for this device
                auto key = udp_key(address);

                // From here on, we are doing things with our target lists that if changed would make us sad
                std::shared_ptr<NetworkTarget> remote;
                /* Mutex scope */ {
                    std::lock_guard<std::mutex> lock(target_mutex);
                    auto r = udp_target.find(key);
                    remote = r == udp_target.end() ? nullptr : r->second;
                }

                switch (header.type) {

                    // A packet announcing that a user is on the network
                    case ANNOUNCE: {
                        // This is an announce packet!
                        const AnnouncePacket& announce = *reinterpret_cast<const AnnouncePacket*>(payload.data());

                        // They're new!
                        if (!remote) {
                            std::string name(&announce.name, payload.size() - sizeof(AnnouncePacket));

                            // If they sent us an empty name ignore that's reserved for multicast transmissions
                            if (!name.empty()) {
                                // Add them into our list
                                auto ptr            = std::make_shared<NetworkTarget>(name, address);
                                bool new_connection = false;
                                /* Mutex scope */ {
                                    std::lock_guard<std::mutex> lock(target_mutex);

                                    // Double check they are new
                                    if (udp_target.count(key) == 0) {
                                        new_connection = true;
                                        targets.push_back(ptr);
                                        udp_target.insert(std::make_pair(key, ptr));
                                        name_target.insert(std::make_pair(name, ptr));

                                        // Say hi back!
                                        ::sendto(data_fd,
                                                 announce_packet.data(),
                                                 static_cast<socklen_t>(announce_packet.size()),
                                                 0,
                                                 &ptr->target.sock,
                                                 socket_size(ptr->target));
                                    }
                                }

                                // Only call the callback if it is new
                                if (new_connection) { join_callback(*ptr); }
                            }
                        }
                        // They're old but at least they're not timing out
                        else {
                            remote->last_update = std::chrono::steady_clock::now();
                        }
                    } break;
                    case LEAVE: {

                        // Goodbye!
                        if (remote) {
                            bool left = false;

                            // Remove from our list
                            /* Mutex scope */ {
                                std::lock_guard<std::mutex> lock(target_mutex);

                                // Double check they are gone after locking before removal
                                if (udp_target.count(key) > 0) {
                                    left = true;
                                    remove_target(remote);
                                }
                            }
                            // Call the callback if they really left
                            if (left) { leave_callback(*remote); }
                        }

                    } break;

                    // A packet containing data
                    case DATA_RETRANSMISSION:
                    case DATA: {

                        // It's a data packet
                        const DataPacket& packet = *reinterpret_cast<const DataPacket*>(payload.data());

                        // If the packet is obviously corrupt, drop it and since we didn't ack it it'll be resent if
                        // it's important
                        if (packet.packet_no > packet.packet_count) { return; }

                        // Check if we know who this is and if we don't know them, ignore
                        if (remote) {

                            // We got a packet from them recently
                            remote->last_update = std::chrono::steady_clock::now();

                            // Check if this packet is a retransmission of data
                            if (header.type == DATA_RETRANSMISSION) {

                                // See if we recently processed this packet
                                auto it = std::find(
                                    remote->recent_packets.begin(), remote->recent_packets.end(), packet.packet_id);

                                // We recently processed this packet, this is just a failed ack
                                // Send the ack again if it was reliable
                                if (it != remote->recent_packets.end() && packet.reliable) {

                                    // Allocate room for the whole ack packet
                                    std::vector<char> r(sizeof(ACKPacket) + (packet.packet_count / 8), 0);
                                    ACKPacket& response   = *reinterpret_cast<ACKPacket*>(r.data());
                                    response              = ACKPacket();
                                    response.packet_id    = packet.packet_id;
                                    response.packet_no    = packet.packet_no;
                                    response.packet_count = packet.packet_count;

                                    // Set the bits for all packets (we got the whole thing)
                                    for (int i = 0; i < packet.packet_count; ++i) {
                                        (&response.packets)[i / 8] |= uint8_t(1 << (i % 8));
                                    }

                                    // Make who we are sending it to into a useable address
                                    sock_t& to = remote->target;

                                    // Send the packet
                                    ::sendto(data_fd,
                                             r.data(),
                                             static_cast<socklen_t>(r.size()),
                                             0,
                                             &to.sock,
                                             socket_size(to));

                                    // We don't need to process this packet we already did
                                    return;
                                }
                            }

                            // If this is a solo packet (in a single chunk)
                            if (packet.packet_count == 1) {

                                // Copy our data into a vector
                                std::vector<char> out(&packet.data,
                                                      &packet.data + payload.size() - sizeof(DataPacket) + 1);

                                // If this is a reliable packet, send an ack back
                                if (packet.reliable) {
                                    // This response is easy since there is only one packet
                                    ACKPacket response;
                                    response.packet_id    = packet.packet_id;
                                    response.packet_no    = packet.packet_no;
                                    response.packet_count = packet.packet_count;
                                    response.packets      = 1;

                                    // Make who we are sending it to into a useable address
                                    sock_t& to = remote->target;

                                    sendto(data_fd,
                                           reinterpret_cast<const char*>(&response),
                                           sizeof(response),
                                           0,
                                           &to.sock,
                                           socket_size(to));

                                    // Set this packet to have been recently received
                                    remote->recent_packets[++remote->recent_packets_index] = packet.packet_id;
                                }

                                packet_callback(*remote, packet.hash, packet.reliable, std::move(out));
                            }
                            else {
                                std::lock_guard<std::mutex> lock(remote->assemblers_mutex);

                                // Grab the payload and put it in our list of assemblers targets
                                auto& assemblers = remote->assemblers;

                                auto& assembler = assemblers[packet.packet_id];

                                // First check that our cache isn't super corrupted by ensuring that our last packet
                                // in our list isn't after the number of packets we have
                                if (!assembler.second.empty()
                                    && std::next(assembler.second.end(), -1)->first >= packet.packet_count) {

                                    // If so, we need to purge our cache and if this was a reliable packet, send a
                                    // NACK back for all the packets we thought we had
                                    // We don't know if we have any packets except the one we just got
                                    if (packet.reliable) {

                                        // A basic ack has room for 8 packets and we need 1 extra byte for each 8
                                        // additional packets
                                        std::vector<char> r(sizeof(NACKPacket) + (packet.packet_count / 8), 0);
                                        NACKPacket& response  = *reinterpret_cast<NACKPacket*>(r.data());
                                        response              = NACKPacket();
                                        response.packet_id    = packet.packet_id;
                                        response.packet_count = packet.packet_count;

                                        // Set the bits for the packets we thought we received
                                        for (auto& p : assembler.second) {
                                            (&response.packets)[p.first / 8] |= uint8_t(1 << (p.first % 8));
                                        }

                                        // Ensure the bit for this packet isn't NACKed
                                        (&response.packets)[packet.packet_no / 8] &=
                                            ~uint8_t(1 << (packet.packet_no % 8));

                                        // Make who we are sending it to into a useable address
                                        sock_t& to = remote->target;

                                        // Send the packet
                                        ::sendto(data_fd,
                                                 r.data(),
                                                 static_cast<socklen_t>(r.size()),
                                                 0,
                                                 &to.sock,
                                                 socket_size(to));
                                    }

                                    // Clear our packets here (the one we just got will be added right after this)
                                    assembler.second.clear();
                                }

                                // Add our packet to our list of assemblers
                                assembler.first                    = std::chrono::steady_clock::now();
                                assembler.second[packet.packet_no] = std::move(payload);

                                // Create and send our ACK packet if this is a reliable transmission
                                if (packet.reliable) {
                                    // A basic ack has room for 8 packets and we need 1 extra byte for each 8
                                    // additional packets
                                    std::vector<char> r(sizeof(ACKPacket) + (packet.packet_count / 8), 0);
                                    ACKPacket& response   = *reinterpret_cast<ACKPacket*>(r.data());
                                    response              = ACKPacket();
                                    response.packet_id    = packet.packet_id;
                                    response.packet_no    = packet.packet_no;
                                    response.packet_count = packet.packet_count;

                                    // Set the bits for the packets we have received
                                    for (auto& p : assembler.second) {
                                        (&response.packets)[p.first / 8] |= uint8_t(1 << (p.first % 8));
                                    }

                                    // Make who we are sending it to into a useable address
                                    sock_t& to = remote->target;

                                    // Send the packet
                                    ::sendto(data_fd,
                                             r.data(),
                                             static_cast<socklen_t>(r.size()),
                                             0,
                                             &to.sock,
                                             socket_size(to));
                                }

                                // Check to see if we have enough to assemble the whole thing
                                if (assembler.second.size() == packet.packet_count) {

                                    // Work out exactly how much data we will need first so we only need one
                                    // allocation
                                    size_t payload_size = 0;
                                    for (auto& p : assembler.second) {
                                        payload_size += p.second.size() - sizeof(DataPacket) + 1;
                                    }

                                    // Read in our data
                                    std::vector<char> out;
                                    out.reserve(payload_size);
                                    for (auto& p : assembler.second) {
                                        const DataPacket& part = *reinterpret_cast<DataPacket*>(p.second.data());
                                        out.insert(out.end(),
                                                   &part.data,
                                                   &part.data + p.second.size() - sizeof(DataPacket) + 1);
                                    }

                                    // Send our assembled data packet
                                    packet_callback(*remote, packet.hash, packet.reliable, std::move(out));

                                    // If the packet was reliable add that it was recently received
                                    if (packet.reliable) {
                                        // Set this packet to have been recently received
                                        remote->recent_packets[++remote->recent_packets_index] = packet.packet_id;
                                    }

                                    // We have completed this packet, discard the data
                                    assemblers.erase(assemblers.find(packet.packet_id));
                                }
                            }
                        }
                    } break;

                    // Packet acknowledging the receipt of a packet of data
                    case ACK: {

                        // It's an ack packet
                        const ACKPacket& packet = *reinterpret_cast<const ACKPacket*>(payload.data());

                        // Check if we know who this is and if we don't know them, ignore
                        if (remote) {

                            // We got a packet from them recently
                            remote->last_update = std::chrono::steady_clock::now();

                            // lock the send queue mutex
                            std::lock_guard<std::mutex> send_lock(send_queue_mutex);

                            // Check for our packet id in the send queue
                            if (send_queue.count(packet.packet_id) > 0) {

                                auto& queue = send_queue[packet.packet_id];

                                // Find this target in the send queue
                                auto s = std::find_if(queue.targets.begin(),
                                                      queue.targets.end(),
                                                      [&](const PacketQueue::PacketTarget& target) {
                                                          return target.target.lock() == remote;
                                                      });

                                // Check for all the ways this ACK could be invalid:
                                // From an unknown person
                                if (s != queue.targets.end()
                                    // Wrong packet
                                    && packet.packet_count == queue.header.packet_count
                                    // Truncated packet
                                    && payload.size() == (sizeof(ACKPacket) + (queue.header.packet_count / 8))) {

                                    // Work out about how long our round trip time is
                                    auto now        = std::chrono::steady_clock::now();
                                    auto round_trip = now - s->last_send;

                                    // Approximate how long the round trip is to this remote so we can work out how
                                    // long before retransmitting
                                    // We use a baby kalman filter to help smooth out jitter
                                    remote->measure_round_trip(round_trip);

                                    // Update our acks
                                    bool all_acked = true;
                                    for (unsigned i = 0; i < s->acked.size(); ++i) {

                                        // Update our bitset
                                        s->acked[i] |= (&packet.packets)[i];

                                        // Work out what a "fully acked" packet would look like
                                        uint8_t expected = i + 1 < s->acked.size() || packet.packet_count % 8 == 0
                                                               ? 0xFF
                                                               : 0xFF >> (8 - (packet.packet_count % 8));

                                        all_acked &= static_cast<int>((s->acked[i] & expected) == expected);
                                    }

                                    // The remote has received this entire packet we can erase our sender
                                    if (all_acked) {
                                        queue.targets.erase(s);

                                        // If we're all done remove the whole thing
                                        if (queue.targets.empty()) { send_queue.erase(packet.packet_id); }
                                    }
                                }
                            }
                        }
                    } break;

                    // Packet requesting a retransmission of some corrupt data
                    case NACK: {
                        // It's a nack packet
                        const NACKPacket& packet = *reinterpret_cast<const NACKPacket*>(payload.data());

                        // Check if we know who this is and if we don't know them, ignore
                        if (remote) {

                            // We got a packet from them recently
                            remote->last_update = std::chrono::steady_clock::now();

                            // Check for our packet id in the send queue
                            if (send_queue.count(packet.packet_id) > 0) {

                                // Find this packet in our sending queue
                                auto& queue = send_queue[packet.packet_id];

                                // Find this target in the send queue
                                auto s = std::find_if(queue.targets.begin(),
                                                      queue.targets.end(),
                                                      [&](const PacketQueue::PacketTarget& target) {
                                                          return target.target.lock() == remote;
                                                      });

                                // Validate that the nack is relevant and valid
                                // We know who it is
                                if (s != queue.targets.end()
                                    // It's not corrupted
                                    && packet.packet_count == queue.header.packet_count
                                    // It's not truncated
                                    && payload.size() == (sizeof(NACKPacket) + (queue.header.packet_count / 8))) {

                                    // Store the time as we are now sending new packets
                                    s->last_send = std::chrono::steady_clock::now();

                                    // The next time we should check for a timeout
                                    auto next_timeout = s->last_send + remote->round_trip_time;
                                    if (next_timeout < next_event) {
                                        next_event = next_timeout;
                                        next_event_callback(next_event);
                                    }

                                    // Update our acks with the nacked data
                                    for (unsigned i = 0; i < s->acked.size(); ++i) {

                                        // Update our bitset
                                        s->acked[i] &= ~(&packet.packets)[i];
                                    }

                                    // Now we have to retransmit the nacked packets
                                    for (uint16_t i = 0; i < packet.packet_count * 8; ++i) {

                                        // Check if this packet needs to be sent
                                        uint8_t bit = 1 << (i % 8);
                                        if (((&packet.packets)[i] & bit) == bit) {
                                            send_packet(remote->target, queue.header, i, queue.payload, true);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }


        std::vector<fd_t> NUClearNetwork::listen_fds() {
            return std::vector<fd_t>({data_fd, announce_fd});
        }

        void NUClearNetwork::send_packet(const sock_t& target,
                                         NUClear::extension::network::DataPacket header,
                                         uint16_t packet_no,
                                         const std::vector<char>& payload,
                                         const bool& /*reliable*/) {

            // Our packet we are sending
            msghdr message;
            std::memset(&message, 0, sizeof(msghdr));

            iovec data[2];
            message.msg_iov    = static_cast<iovec*>(data);
            message.msg_iovlen = 2;

            // Update our headers packet number and set it in the message
            header.packet_no = packet_no;
            data[0].iov_base = reinterpret_cast<char*>(&header);
            data[0].iov_len  = sizeof(DataPacket) - 1;

            // Work out what chunk of data we are sending const cast is fine as posix guarantees it won't be
            // modified
            data[1].iov_base = const_cast<char*>(payload.data() + (packet_data_mtu * packet_no));  // NOLINT
            data[1].iov_len  = packet_no + 1 < header.packet_count ? packet_data_mtu : payload.size() % packet_data_mtu;

            // Set our target and send (once again const cast is fine)
            message.msg_name    = const_cast<sockaddr*>(&target.sock);  // NOLINT
            message.msg_namelen = socket_size(target);

            // TODO(trent): if reliable, run select first to see if this socket is writeable
            // If it is not reliable just don't send the message instead of blocking
            sendmsg(data_fd, &message, 0);
        }


        void NUClearNetwork::send(const uint64_t& hash,
                                  const std::vector<char>& payload,
                                  const std::string& target,
                                  bool reliable) {

            // If we are not connected throw an error
            if (targets.empty()) { throw std::runtime_error("Cannot send messages as the network is not connected"); }


            // The header for our packet
            DataPacket header;

            /* Mutex Scope */ {
                std::lock_guard<std::mutex> lock(send_queue_mutex);
                // For the packet id we ensure that it's not currently used for retransmission
                while (send_queue.count(header.packet_id = ++packet_id_source) > 0) {}
            }

            header.packet_no    = 0;
            header.packet_count = uint16_t((payload.size() / packet_data_mtu) + 1);
            header.reliable     = reliable;
            header.hash         = hash;

            // If this was a reliable packet we need to cache it in case it needs to be resent
            if (reliable) {

                std::lock_guard<std::mutex> lock_target(target_mutex);
                std::lock_guard<std::mutex> lock_send(send_queue_mutex);

                auto& queue = send_queue[header.packet_id];

                // Store the header, but update it's type to be a retransmission so it can be ignored if
                // overtransmitted
                queue.header      = header;
                queue.header.type = DATA_RETRANSMISSION;
                // TODO(trent): there might be some better memory management that can happen here
                queue.payload = payload;
                std::vector<uint8_t> acks((header.packet_count / 8) + 1, 0);

                // Find interested parties or if multicast it's everyone we are connected to
                auto range = target.empty() ? std::make_pair(name_target.begin(), name_target.end())
                                            : name_target.equal_range(target);
                for (auto it = range.first; it != range.second; ++it) {
                    // If this target is an announce target ignore it
                    if (it->first != "") {
                        // Add this guy to the queue
                        queue.targets.emplace_back(it->second, acks);

                        // The next time we should check for a timeout
                        auto next_timeout = std::chrono::steady_clock::now() + it->second->round_trip_time;
                        if (next_timeout < next_event) {
                            next_event = next_timeout;
                            next_event_callback(next_event);
                        }
                    }
                }
            }

            /* Mutex Scope */ {
                std::lock_guard<std::mutex> lock(target_mutex);

                // Now send all our packets to our targets
                auto send_to = name_target.equal_range(target);
                for (uint16_t i = 0; i < header.packet_count; ++i) {
                    for (auto s = send_to.first; s != send_to.second; ++s) {
                        send_packet(s->second->target, header, i, payload, reliable);
                    }
                }
            }
        }

    }  // namespace network
}  // namespace extension
}  // namespace NUClear
