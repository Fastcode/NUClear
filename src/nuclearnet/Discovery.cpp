/*
    * MIT License
    *
    * Copyright (c) 2025 NUClear Contributors
    *
    * This file is part of the NUClear codebase.
    * See https://github.com/Fastcode/NUClear for further info.
    *
    * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
    * documentation files (the "Software"), to deal in the Software without restriction, including without limitation
    * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
    * to permit persons to whom the Software is furnished to do so, subject to the following conditions:
    *
    * The above copyright notice and this permission notice shall be included in all copies or substantial portions of
    * the Software.
    *
    * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    * THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    * IN THE SOFTWARE.
    */

#include "Discovery.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "wire_protocol.hpp"

    namespace NUClear {
    namespace network {

        Discovery::Discovery(std::chrono::steady_clock::duration peer_timeout) : peer_timeout(peer_timeout) {}

        void Discovery::set_join_callback(JoinCallback cb) {
            join_callback = std::move(cb);
        }

        void Discovery::set_leave_callback(LeaveCallback cb) {
            leave_callback = std::move(cb);
        }

        void Discovery::set_subscription_change_callback(SubscriptionChangeCallback cb) {
            subscription_change_callback = std::move(cb);
        }

        std::vector<uint8_t> Discovery::build_announce_packet(const std::string& name,
                                                              const std::vector<uint64_t>& subscriptions) {
            // Calculate total size:
            // PacketHeader(5) + name_length(2) + name(variable) + num_subscriptions(2) + subscriptions(8 each)
            const std::size_t size = sizeof(PacketHeader) + sizeof(uint16_t) + name.size() + sizeof(uint16_t)
                                     + subscriptions.size() * sizeof(uint64_t);

            std::vector<uint8_t> packet(size);
            uint8_t* ptr = packet.data();

            // Write header
            PacketHeader header(ANNOUNCE);
            std::memcpy(ptr, &header, sizeof(PacketHeader));
            ptr += sizeof(PacketHeader);

            // Write name length and name
            auto name_len = static_cast<uint16_t>(name.size());
            std::memcpy(ptr, &name_len, sizeof(uint16_t));
            ptr += sizeof(uint16_t);
            std::memcpy(ptr, name.data(), name.size());
            ptr += name.size();

            // Write subscription count and hashes
            auto num_subs = static_cast<uint16_t>(subscriptions.size());
            std::memcpy(ptr, &num_subs, sizeof(uint16_t));
            ptr += sizeof(uint16_t);
            for (const auto& hash : subscriptions) {
                std::memcpy(ptr, &hash, sizeof(uint64_t));
                ptr += sizeof(uint64_t);
            }

            return packet;
        }

        std::vector<uint8_t> Discovery::build_leave_packet() {
            std::vector<uint8_t> packet(sizeof(LeavePacket));
            LeavePacket leave;
            std::memcpy(packet.data(), &leave, sizeof(LeavePacket));
            return packet;
        }

        std::vector<uint8_t> Discovery::build_connect_packet(uint8_t flags) {
            std::vector<uint8_t> packet(sizeof(ConnectPacket));
            ConnectPacket connect;
            connect.flags = flags;
            std::memcpy(packet.data(), &connect, sizeof(ConnectPacket));
            return packet;
        }

        void Discovery::process_announce(const sock_t& source,
                                          const uint8_t* data,
                                          std::size_t length,
                                          std::chrono::steady_clock::time_point now) {
            // Minimum size: header(5) + name_length(2) + num_subscriptions(2) = 9
            if (length < sizeof(PacketHeader) + sizeof(uint16_t) + sizeof(uint16_t)) {
                return;
            }

            const uint8_t* ptr    = data + sizeof(PacketHeader);
            std::size_t remaining = length - sizeof(PacketHeader);

            // Read name length
            uint16_t name_len = 0;
            std::memcpy(&name_len, ptr, sizeof(uint16_t));
            ptr += sizeof(uint16_t);
            remaining -= sizeof(uint16_t);

            // Validate name fits
            if (remaining < name_len + sizeof(uint16_t)) {
                return;
            }

            // Read name
            std::string name(reinterpret_cast<const char*>(ptr), name_len);
            ptr += name_len;
            remaining -= name_len;

            // Ignore empty names
            if (name.empty()) {
                return;
            }

            // Read subscription count
            uint16_t num_subs = 0;
            std::memcpy(&num_subs, ptr, sizeof(uint16_t));
            ptr += sizeof(uint16_t);
            remaining -= sizeof(uint16_t);

            // Validate subscriptions fit
            if (remaining < num_subs * sizeof(uint64_t)) {
                return;
            }

            // Read subscriptions
            std::set<uint64_t> subscriptions;
            for (uint16_t i = 0; i < num_subs; ++i) {
                uint64_t hash = 0;
                std::memcpy(&hash, ptr, sizeof(uint64_t));
                ptr += sizeof(uint64_t);
                subscriptions.insert(hash);
            }

            // Check if this is a new peer or an existing one
            bool subs_changed = false;
            bool fire_join    = false;
            PeerInfo join_info;
            {
                const std::lock_guard<std::mutex> lock(peers_mutex);

                auto it = peers.find(source);
                if (it == peers.end()) {
                    // New peer — record with announce_heard = true
                    PeerInfo info;
                    info.name            = name;
                    info.address         = source;
                    info.last_seen       = now;
                    info.subscriptions   = std::move(subscriptions);
                    info.announce_heard  = true;
                    info.handshake       = HandshakeState::IDLE;
                    peers.emplace(source, std::move(info));
                }
                else {
                    auto& peer = it->second;
                    peer.last_seen = now;

                    // Mark announce as heard (may trigger connection if data was already confirmed)
                    if (!peer.announce_heard) {
                        peer.announce_heard = true;
                        if (peer.handshake == HandshakeState::CONFIRMED) {
                            fire_join = true;
                            join_info = peer;
                        }
                    }

                    // Update name if it was unknown (peer added via CONNECT before announce)
                    if (peer.name.empty()) {
                        peer.name = name;
                    }

                    // Check for subscription changes
                    if (peer.subscriptions != subscriptions) {
                        peer.subscriptions = std::move(subscriptions);
                        subs_changed       = true;
                    }
                }
            }

            // Fire callbacks outside the lock
            if (fire_join && join_callback) {
                join_callback(join_info);
            }
            if (subs_changed && subscription_change_callback) {
                PeerInfo info;
                {
                    const std::lock_guard<std::mutex> lock(peers_mutex);
                    auto it = peers.find(source);
                    if (it != peers.end()) {
                        info = it->second;
                    }
                    else {
                        subs_changed = false;
                    }
                }
                if (subs_changed) {
                    subscription_change_callback(info);
                }
            }
        }

        void Discovery::process_leave(const sock_t& source) {
            PeerInfo removed;
            bool was_connected = false;
            {
                const std::lock_guard<std::mutex> lock(peers_mutex);
                auto it = peers.find(source);
                if (it != peers.end()) {
                    was_connected = (it->second.announce_heard
                                     && it->second.handshake == HandshakeState::CONFIRMED);
                    removed       = it->second;
                    peers.erase(it);
                }
            }

            // Only fire leave callback for peers that completed the handshake
            if (was_connected && leave_callback) {
                leave_callback(removed);
            }
        }

        Discovery::ConnectResult Discovery::process_connect(const sock_t& source,
                                                            uint8_t flags,
                                                            std::chrono::steady_clock::time_point now) {
            ConnectResult result;
            bool fire_join = false;
            PeerInfo info;

            {
                const std::lock_guard<std::mutex> lock(peers_mutex);
                auto it = peers.find(source);
                if (it == peers.end()) {
                    // Unknown peer — add with minimal info (name/subs will come from announce)
                    PeerInfo new_peer;
                    new_peer.address        = source;
                    new_peer.last_seen      = now;
                    new_peer.announce_heard = false;
                    new_peer.handshake      = HandshakeState::IDLE;
                    it = peers.emplace(source, std::move(new_peer)).first;
                }

                auto& peer  = it->second;
                peer.last_seen = now;

                const bool has_syn = (flags & SYN) != 0;
                const bool has_ack = (flags & CON_ACK) != 0;

                switch (peer.handshake) {
                    case HandshakeState::IDLE:
                        if (has_syn && !has_ack) {
                            // Received SYN — respond with SYN+ACK
                            peer.handshake        = HandshakeState::SYN_RECEIVED;
                            result.response_flags = SYN | CON_ACK;
                        }
                        else if (has_syn && has_ack) {
                            // SYN+ACK but we haven't sent SYN — treat as SYN, respond SYN+ACK
                            peer.handshake        = HandshakeState::SYN_RECEIVED;
                            result.response_flags = SYN | CON_ACK;
                        }
                        break;

                    case HandshakeState::SYN_SENT:
                        if (has_syn && has_ack) {
                            // Received SYN+ACK to our SYN — send ACK, data path confirmed
                            peer.handshake        = HandshakeState::CONFIRMED;
                            result.response_flags = CON_ACK;
                            if (peer.announce_heard) {
                                result.just_connected = true;
                                fire_join             = true;
                                info                  = peer;
                            }
                        }
                        else if (has_syn && !has_ack) {
                            // Simultaneous open: both sent SYN at the same time
                            // Respond with SYN+ACK
                            peer.handshake        = HandshakeState::SYN_RECEIVED;
                            result.response_flags = SYN | CON_ACK;
                        }
                        break;

                    case HandshakeState::SYN_RECEIVED:
                        if (has_ack && !has_syn) {
                            // Received ACK to our SYN+ACK — data path confirmed
                            peer.handshake = HandshakeState::CONFIRMED;
                            if (peer.announce_heard) {
                                result.just_connected = true;
                                fire_join             = true;
                                info                  = peer;
                            }
                        }
                        else if (has_syn && has_ack) {
                            // Simultaneous open: both in SYN_RECEIVED, received SYN+ACK
                            peer.handshake        = HandshakeState::CONFIRMED;
                            result.response_flags = CON_ACK;
                            if (peer.announce_heard) {
                                result.just_connected = true;
                                fire_join             = true;
                                info                  = peer;
                            }
                        }
                        break;

                    case HandshakeState::CONFIRMED:
                        // Already confirmed — respond to duplicates
                        if (has_syn && has_ack) {
                            result.response_flags = CON_ACK;
                        }
                        else if (has_syn && !has_ack) {
                            // Peer might have restarted — respond with SYN+ACK
                            result.response_flags = SYN | CON_ACK;
                        }
                        break;
                }
            }

            // Fire join callback outside the lock
            if (fire_join && join_callback) {
                join_callback(info);
            }

            return result;
        }

        void Discovery::mark_syn_sent(const sock_t& address) {
            const std::lock_guard<std::mutex> lock(peers_mutex);
            auto it = peers.find(address);
            if (it != peers.end() && it->second.handshake == HandshakeState::IDLE) {
                it->second.handshake = HandshakeState::SYN_SENT;
            }
        }

        bool Discovery::is_connected(const sock_t& address) const {
            const std::lock_guard<std::mutex> lock(peers_mutex);
            auto it = peers.find(address);
            return it != peers.end() && it->second.announce_heard
                   && it->second.handshake == HandshakeState::CONFIRMED;
        }

        void Discovery::touch_peer(const sock_t& source, std::chrono::steady_clock::time_point now) {
            const std::lock_guard<std::mutex> lock(peers_mutex);
            auto it = peers.find(source);
            if (it != peers.end()) {
                it->second.last_seen = now;
            }
        }

        std::vector<PeerInfo> Discovery::check_timeouts(std::chrono::steady_clock::time_point now) {
            std::vector<PeerInfo> removed;

            {
                const std::lock_guard<std::mutex> lock(peers_mutex);
                for (auto it = peers.begin(); it != peers.end();) {
                    if (now - it->second.last_seen > peer_timeout) {
                        // Only report leave for peers that were fully connected
                        if (it->second.announce_heard
                            && it->second.handshake == HandshakeState::CONFIRMED) {
                            removed.push_back(it->second);
                        }
                        it = peers.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }

            // Fire leave callbacks outside the lock
            if (leave_callback) {
                for (const auto& peer : removed) {
                    leave_callback(peer);
                }
            }

            return removed;
        }

        std::map<Discovery::sock_t, PeerInfo> Discovery::get_peers() const {
            const std::lock_guard<std::mutex> lock(peers_mutex);
            return peers;
        }

        bool Discovery::has_peer(const sock_t& address) const {
            const std::lock_guard<std::mutex> lock(peers_mutex);
            return peers.count(address) > 0;
        }

        const PeerInfo* Discovery::get_peer(const sock_t& address) const {
            const std::lock_guard<std::mutex> lock(peers_mutex);
            auto it = peers.find(address);
            if (it != peers.end()) {
                return &it->second;
            }
            return nullptr;
        }

    }  // namespace network
}  // namespace NUClear
