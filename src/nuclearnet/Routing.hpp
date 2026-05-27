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

#ifndef NUCLEAR_NETWORK_ROUTING_HPP
#define NUCLEAR_NETWORK_ROUTING_HPP

#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <vector>

#include "../util/network/sock_t.hpp"

namespace NUClear {
namespace network {

    /**
     * Manages subscription-based message routing.
     *
     * Responsibilities:
     * - Tracking per-peer subscriptions (which message types each peer wants)
     * - Filtering outgoing messages based on subscriptions
     * - Managing this node's own subscription list for announce packets
     *
     * Default behavior: if a peer has no subscriptions (empty set), send all messages to it.
     * This ensures backwards-compatible behavior with peers that don't support filtering.
     */
    class Routing {
    public:
        using sock_t = util::network::sock_t;

        /**
         * Update the subscription set for a remote peer.
         *
         * @param peer          The peer's address
         * @param subscriptions The set of hashes the peer wants (empty = all)
         */
        void update_peer_subscriptions(const sock_t& peer, std::set<uint64_t> subscriptions);

        /**
         * Remove a peer's subscription information (e.g., on disconnect).
         *
         * @param peer The peer to remove
         */
        void remove_peer(const sock_t& peer);

        /**
         * Check whether a message with the given hash should be sent to a specific peer.
         *
         * @param peer The target peer
         * @param hash The message type hash
         *
         * @return true if the message should be sent (peer subscribes to it, or has no filter)
         */
        bool should_send(const sock_t& peer, uint64_t hash) const;

        /**
         * Get the list of peers that should receive a message with the given hash.
         *
         * @param all_peers All known peer addresses
         * @param hash      The message type hash
         *
         * @return Subset of peers that should receive this message
         */
        std::vector<sock_t> get_targets(const std::vector<sock_t>& all_peers, uint64_t hash) const;

        /**
         * Set this node's local subscriptions (what we want to receive).
         * This is used when building announce packets.
         *
         * @param subscriptions The set of hashes this node wants to receive (empty = all)
         */
        void set_local_subscriptions(std::set<uint64_t> subscriptions);

        /**
         * Get this node's local subscriptions as a vector (for building announce packets).
         *
         * @return The subscription hashes
         */
        std::vector<uint64_t> get_local_subscriptions() const;

        /**
         * Add a single hash to the local subscriptions.
         *
         * @param hash The message type hash to subscribe to
         */
        void add_local_subscription(uint64_t hash);

        /**
         * Remove a single hash from the local subscriptions.
         *
         * @param hash The message type hash to unsubscribe from
         */
        void remove_local_subscription(uint64_t hash);

        /**
         * Check if we are locally subscribed to a message type hash.
         * Returns true if we have no subscriptions (empty = receive all) or if the hash is in our set.
         *
         * @param hash The message type hash to check
         *
         * @return true if we should accept this message type
         */
        bool is_locally_subscribed(uint64_t hash) const;

    private:
        /// Per-peer subscription sets (empty set = send all)
        mutable std::mutex peer_mutex;
        std::map<sock_t, std::set<uint64_t>> peer_subscriptions;

        /// This node's own subscriptions
        mutable std::mutex local_mutex;
        std::set<uint64_t> local_subscriptions;
    };

}  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_ROUTING_HPP
