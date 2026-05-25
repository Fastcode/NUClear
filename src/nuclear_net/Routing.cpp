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

#include "Routing.hpp"

namespace NUClear {
namespace network {

    void Routing::update_peer_subscriptions(const sock_t& peer, std::set<uint64_t> subscriptions) {
        const std::lock_guard<std::mutex> lock(peer_mutex);
        peer_subscriptions[peer] = std::move(subscriptions);
    }

    void Routing::remove_peer(const sock_t& peer) {
        const std::lock_guard<std::mutex> lock(peer_mutex);
        peer_subscriptions.erase(peer);
    }

    bool Routing::should_send(const sock_t& peer, uint64_t hash) const {
        const std::lock_guard<std::mutex> lock(peer_mutex);
        auto it = peer_subscriptions.find(peer);
        if (it == peer_subscriptions.end()) {
            // Unknown peer — default to sending
            return true;
        }
        // Empty subscription set means "send everything"
        if (it->second.empty()) {
            return true;
        }
        // Check if the hash is in the peer's subscription set
        return it->second.count(hash) > 0;
    }

    std::vector<Routing::sock_t> Routing::get_targets(const std::vector<sock_t>& all_peers, uint64_t hash) const {
        std::vector<sock_t> targets;
        targets.reserve(all_peers.size());
        for (const auto& peer : all_peers) {
            if (should_send(peer, hash)) {
                targets.push_back(peer);
            }
        }
        return targets;
    }

    void Routing::set_local_subscriptions(std::set<uint64_t> subscriptions) {
        const std::lock_guard<std::mutex> lock(local_mutex);
        local_subscriptions = std::move(subscriptions);
    }

    std::vector<uint64_t> Routing::get_local_subscriptions() const {
        const std::lock_guard<std::mutex> lock(local_mutex);
        return {local_subscriptions.begin(), local_subscriptions.end()};
    }

    void Routing::add_local_subscription(uint64_t hash) {
        const std::lock_guard<std::mutex> lock(local_mutex);
        local_subscriptions.insert(hash);
    }

    void Routing::remove_local_subscription(uint64_t hash) {
        const std::lock_guard<std::mutex> lock(local_mutex);
        local_subscriptions.erase(hash);
    }

}  // namespace network
}  // namespace NUClear
