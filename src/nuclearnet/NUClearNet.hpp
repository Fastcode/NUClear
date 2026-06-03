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

#ifndef NUCLEAR_NETWORK_NUCLEARNET_HPP
#define NUCLEAR_NETWORK_NUCLEARNET_HPP

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "../util/network/sock_t.hpp"
#include "Discovery.hpp"
#include "FileDescriptor.hpp"
#include "Log.hpp"
#include "Fragmentation.hpp"
#include "PacketDeduplicator.hpp"
#include "Reliability.hpp"
#include "Routing.hpp"

namespace NUClear {
namespace network {

    /**
     * Configuration for the NUClearNet networking system.
     */
    struct NetworkConfig {
        /// This node's name on the network
        std::string name;
        /// The multicast/broadcast/unicast address to announce on (organization-local multicast default)
        std::string announce_address = "239.226.152.162";  // NOSONAR
        /// The port to use for announce discovery
        in_port_t announce_port = 7447;
        /// Address to bind to (empty = all interfaces)
        std::string bind_address;
        /// Network MTU (used to calculate fragment size)
        uint16_t mtu = 1500;
        /// Peer timeout duration
        std::chrono::steady_clock::duration peer_timeout = std::chrono::seconds(2);
        /// Maximum total assembly size for fragmented messages
        std::size_t max_assembly_size = 64 * 1024 * 1024;
    };

    /**
     * NUClearNet — standalone UDP networking library for peer-to-peer communication.
     *
     * Provides:
     * - Automatic peer discovery via multicast/broadcast announces
     * - NAT-friendly port learning (from UDP source address)
     * - Fragmentation and reassembly of large messages
     * - Optional reliable delivery with ACK-based retransmission
     * - Subscription-based message filtering
     * - Per-peer RTT estimation with Jacobson/Karels algorithm
     *
     * This class can be used independently of the NUClear reactor framework.
     * It operates via a poll-based model: call process() to handle network events.
     */
    class NUClearNet {
    public:
        using sock_t = util::network::sock_t;

        /// Callback for received complete messages
        using PacketCallback = std::function<void(const sock_t& source,
                                                  const std::string& peer_name,
                                                  uint64_t hash,
                                                  bool reliable,
                                                  std::vector<uint8_t>&& payload)>;

        /// Callback for peer join events
        using JoinCallback = std::function<void(const PeerInfo&)>;

        /// Callback for peer leave events
        using LeaveCallback = std::function<void(const PeerInfo&)>;

        /// Callback for when the system needs attention at a specific time
        using EventCallback = std::function<void(std::chrono::steady_clock::time_point)>;

        /// Callback invoked when sockets are replaced so the caller can update IO event registrations
        using SocketChangeCallback = std::function<void()>;

        NUClearNet();
        ~NUClearNet();
        NUClearNet(const NUClearNet&)            = delete;
        NUClearNet(NUClearNet&&)                 = delete;
        NUClearNet& operator=(const NUClearNet&) = delete;
        NUClearNet& operator=(NUClearNet&&)      = delete;

        /**
         * Reset/configure the network with new settings.
         * If already running, shuts down first, then reinitializes.
         *
         * @param config The network configuration
         */
        void reset(const NetworkConfig& config);

        /**
         * Shut down the network, sending a leave packet and closing sockets.
         */
        void shutdown();

        /**
         * Set the global NUClearNet log level (stderr). Default is Off.
         */
        static void set_log_level(LogLevel level);

        /**
         * Process pending network events (send announces, check timeouts, read packets).
         * Call this periodically or when a file descriptor becomes readable.
         */
        void process();

        /**
         * Send a message to the network.
         *
         * @param hash     The message type hash
         * @param payload  Pointer to the serialized message data
         * @param length   Length of the payload in bytes
         * @param target   Target peer name (empty = send to all eligible peers)
         * @param reliable Whether to use reliable delivery
         */
        void send(uint64_t hash,
                  const uint8_t* payload,
                  std::size_t length,
                  const std::string& target,
                  bool reliable);

        /**
         * Set this node's subscriptions (which message types to receive).
         * This information is advertised in announce packets.
         *
         * @param subscriptions Set of message type hashes to subscribe to (empty = receive all)
         */
        void set_subscriptions(const std::set<uint64_t>& subscriptions);

        /**
         * Add a single subscription.
         *
         * @param hash The message type hash to subscribe to
         */
        void add_subscription(uint64_t hash);

        // Callback setters
        void set_packet_callback(PacketCallback cb);
        void set_join_callback(JoinCallback cb);
        void set_leave_callback(LeaveCallback cb);
        void set_event_callback(EventCallback cb);

        /**
         * Set a callback to be invoked when sockets are replaced after a rebind.
         * Use this to re-register IO event handlers for the new file descriptors.
         */
        void set_socket_change_callback(SocketChangeCallback cb);

        /**
         * Get the file descriptors that should be monitored for read events.
         * When any of these become readable, call process().
         *
         * @return Vector of file descriptors to monitor
         */
        std::vector<fd_t> listen_fds() const;

        /// Process a single received packet (dispatches to per-type handlers)
        void process_packet(const sock_t& source, const uint8_t* data, std::size_t length);

        /// Process an ANNOUNCE packet from a peer
        void process_announce_packet(const sock_t& source, const uint8_t* data, std::size_t length);

        /// Process a LEAVE packet from a peer
        void process_leave_packet(const sock_t& source);

        /// Process a CONNECT packet from a peer
        void process_connect_packet(const sock_t& source, const uint8_t* data, std::size_t length);

        /// Process a DATA packet from a peer
        void process_data_packet(const sock_t& source, const uint8_t* data, std::size_t length);

        /// Process an ACK packet from a peer
        void process_ack_packet(const sock_t& source, const uint8_t* data, std::size_t length);

    private:
        /// Send an announce packet to the announce address
        void announce();

        /// Open (or reopen) the data and announce sockets using the stored config
        void open_sockets();

        /// Wire discovery module callbacks to NUClearNet handlers
        void wire_discovery_callbacks();

        /// Read and process all pending packets from a socket
        void read_socket(fd_t fd);

        /// Send raw bytes to a target using scatter IO (multiple buffers without copying)
        void send_iov(fd_t fd, const sock_t& target, const iovec* iov, int iovcnt);

        /// Send a single contiguous buffer to a target
        void send_buf(fd_t fd, const sock_t& target, const uint8_t* data, std::size_t length) {
            std::array<iovec, 1> iov{};
#ifdef _WIN32
            iov[0].buf = reinterpret_cast<CHAR*>(const_cast<uint8_t*>(data));  // NOLINT(cppcoreguidelines-pro-type-const-cast)
            iov[0].len = static_cast<ULONG>(length);
#else
            iov[0].iov_base = const_cast<char*>(reinterpret_cast<const char*>(data));  // NOLINT(cppcoreguidelines-pro-type-const-cast,cppcoreguidelines-pro-type-reinterpret-cast)
            iov[0].iov_len  = static_cast<decltype(iov[0].iov_len)>(length);
#endif
            send_iov(fd, target, iov.data(), 1);
        }

        // Configuration
        NetworkConfig config;
        std::string node_name;

        // Sockets
        FileDescriptor data_fd;      ///< Data socket (ephemeral port, sends announces + data)
        FileDescriptor announce_fd;  ///< Announce socket (known port, receives announces)

        // The announce target address
        sock_t announce_target{};

        // Modules
        std::unique_ptr<Discovery> discovery;
        std::unique_ptr<Fragmentation> fragmentation;
        std::unique_ptr<Reliability> reliability;
        Routing routing;

        // Per-peer deduplication
        std::map<sock_t, PacketDeduplicator> deduplicators;

        // Packet ID source (monotonically increasing)
        uint16_t next_packet_id{0};

        // Timing
        std::chrono::steady_clock::time_point last_announce;
        static const std::chrono::milliseconds ANNOUNCE_INTERVAL;

        // Whether the sockets need to be reopened at the start of the next process() call
        bool needs_rebind{false};

        // Callbacks
        PacketCallback packet_callback;
        JoinCallback join_callback;
        LeaveCallback leave_callback;
        EventCallback event_callback;
        SocketChangeCallback socket_change_callback;
    };

}  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_NUCLEARNET_HPP
