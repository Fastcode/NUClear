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

#ifndef NUCLEAR_NETWORK_DISCOVERY_HPP
#define NUCLEAR_NETWORK_DISCOVERY_HPP

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include "../util/network/sock_t.hpp"

namespace NUClear {
namespace network {

    /**
     * Information about a discovered peer on the network.
     */
    struct PeerInfo {
        /// The peer's announced name
        std::string name;
        /// The peer's socket address (IP + data port, learned from UDP source)
        util::network::sock_t address{};
        /// When we last received any packet from this peer
        std::chrono::steady_clock::time_point last_seen;
        /// The set of message type hashes this peer has subscribed to (empty = wants all)
        std::set<uint64_t> subscriptions;
    };

    /**
     * Handles peer discovery via periodic announce packets.
     *
     * Responsibilities:
     * - Sending announce packets (from the data socket for NAT-friendliness)
     * - Processing received announce packets to discover peers
     * - Tracking peer liveness via last-seen timestamps
     * - Removing peers that have timed out
     * - Processing LEAVE packets for graceful departure
     * - Storing per-peer subscription information from announce packets
     */
    class Discovery {
    public:
        using sock_t = util::network::sock_t;

        /// Callback when a new peer joins
        using JoinCallback = std::function<void(const PeerInfo&)>;
        /// Callback when a peer leaves (timeout or graceful)
        using LeaveCallback = std::function<void(const PeerInfo&)>;
        /// Callback when a peer's subscriptions change
        using SubscriptionChangeCallback = std::function<void(const PeerInfo&)>;

        /**
         * Construct the discovery module.
         *
         * @param peer_timeout How long without receiving a packet before a peer is considered gone
         */
        explicit Discovery(std::chrono::steady_clock::duration peer_timeout = std::chrono::seconds(2));

        /**
         * Set the callback to invoke when a new peer joins.
         */
        void set_join_callback(JoinCallback cb);

        /**
         * Set the callback to invoke when a peer leaves.
         */
        void set_leave_callback(LeaveCallback cb);

        /**
         * Set the callback to invoke when a peer's subscriptions change.
         */
        void set_subscription_change_callback(SubscriptionChangeCallback cb);

        /**
         * Build an announce packet for this node.
         *
         * @param name           This node's name
         * @param subscriptions  The set of type hashes this node subscribes to (empty = all)
         *
         * @return The serialized announce packet bytes
         */
        static std::vector<uint8_t> build_announce_packet(const std::string& name,
                                                          const std::vector<uint64_t>& subscriptions);

        /**
         * Build a leave packet.
         *
         * @return The serialized leave packet bytes
         */
        static std::vector<uint8_t> build_leave_packet();

        /**
         * Process a received announce packet from a peer.
         *
         * @param source  The UDP source address (IP + port) of the packet
         * @param data    The raw packet data
         * @param length  The length of the packet data
         * @param now     The current time (defaults to steady_clock::now())
         */
        void process_announce(const sock_t& source,
                              const uint8_t* data,
                              std::size_t length,
                              std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

        /**
         * Process a received leave packet from a peer.
         *
         * @param source The UDP source address of the packet
         */
        void process_leave(const sock_t& source);

        /**
         * Update the last_seen timestamp for a peer (called on any received packet).
         *
         * @param source The UDP source address
         * @param now    The current time (defaults to steady_clock::now())
         */
        void touch_peer(const sock_t& source,
                        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

        /**
         * Check for peers that have timed out and remove them.
         *
         * @param now The current time (defaults to steady_clock::now())
         *
         * @return List of peers that were removed due to timeout
         */
        std::vector<PeerInfo> check_timeouts(
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

        /**
         * Get the current list of known peers.
         *
         * @return Map of socket address to peer info
         */
        std::map<sock_t, PeerInfo> get_peers() const;

        /**
         * Check if a specific peer is known.
         *
         * @param address The peer's address
         * @return true if the peer is known
         */
        bool has_peer(const sock_t& address) const;

        /**
         * Get a specific peer's info.
         *
         * @param address The peer's address
         * @return Pointer to peer info, or nullptr if not found
         */
        const PeerInfo* get_peer(const sock_t& address) const;

    private:
        /// How long without hearing from a peer before it's removed
        std::chrono::steady_clock::duration peer_timeout;

        /// Mutex protecting the peers map
        mutable std::mutex peers_mutex;

        /// Known peers indexed by their socket address
        std::map<sock_t, PeerInfo> peers;

        /// Callbacks
        JoinCallback join_callback;
        LeaveCallback leave_callback;
        SubscriptionChangeCallback subscription_change_callback;
    };

}  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_DISCOVERY_HPP
