/*
        * MIT License
        *
        * Copyright (c) 2025 NUClear Contributors
        *
        * This file is part of the NUClear codebase.
        * See https://github.com/Fastcode/NUClear for further info.
        *
        * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
        * documentation files (the "Software"), to deal in the Software without restriction, including without
        * limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
        * Software, and to permit persons to whom the Software is furnished to do so, subject to the following
        * conditions:
        *
        * The above copyright notice and this permission notice shall be included in all copies or substantial portions
        * of the Software.
        *
        * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
        * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
        * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
        * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
        * DEALINGS IN THE SOFTWARE.
        */

#include "Reliability.hpp"

#include <algorithm>
#include <cstring>

#include "wire_protocol.hpp"

    namespace NUClear {
    namespace network {

        Reliability::Reliability(uint16_t max_retransmits) : max_retransmits(max_retransmits) {}

        void Reliability::track_packet(const sock_t& target,
                                       uint16_t packet_id,
                                       uint16_t packet_count,
                                       uint64_t hash,
                                       uint8_t flags,
                                       const uint8_t* payload,
                                       std::size_t payload_len) {
            TrackedPacket tp;
            tp.target       = target;
            tp.packet_id    = packet_id;
            tp.packet_count = packet_count;
            tp.hash         = hash;
            tp.flags        = flags;
            tp.payload.assign(payload, payload + payload_len);
            tp.acked.resize(packet_count, false);
            tp.last_send = std::chrono::steady_clock::now();

            TrackingKey key{target, packet_id};

            const std::lock_guard<std::mutex> lock(tracking_mutex);
            tracked_packets[key] = std::move(tp);
        }

        void Reliability::process_ack(const sock_t& source,
                                      uint16_t packet_id,
                                      uint16_t packet_count,
                                      const uint8_t* ack_bitset,
                                      std::size_t bitset_size) {
            TrackingKey key{source, packet_id};

            const std::lock_guard<std::mutex> lock(tracking_mutex);
            auto it = tracked_packets.find(key);
            if (it == tracked_packets.end()) {
                return;
            }

            auto& tp = it->second;

            // Update RTT estimate based on time since last send
            auto now = std::chrono::steady_clock::now();
            auto rtt = now - tp.last_send;
            {
                const std::lock_guard<std::mutex> rtt_lock(rtt_mutex);
                rtt_estimators[source].measure(rtt);
            }

            // Update acked bitset
            bool all_acked = true;
            for (uint16_t i = 0; i < packet_count && i < tp.acked.size(); ++i) {
                std::size_t byte_idx = i / 8;
                uint8_t bit_idx      = i % 8;
                if (byte_idx < bitset_size && (ack_bitset[byte_idx] & (1u << bit_idx)) != 0) {
                    tp.acked[i] = true;
                }
                if (!tp.acked[i]) {
                    all_acked = false;
                }
            }

            // If all fragments are ACKed, remove from tracking
            if (all_acked) {
                tracked_packets.erase(it);
            }
        }

        void Reliability::process_nack(const sock_t& source,
                                       uint16_t packet_id,
                                       uint16_t /*packet_count*/,
                                       const uint8_t* /*nack_bitset*/,
                                       std::size_t /*bitset_size*/) {
            TrackingKey key{source, packet_id};

            const std::lock_guard<std::mutex> lock(tracking_mutex);
            auto it = tracked_packets.find(key);
            if (it == tracked_packets.end()) {
                return;
            }

            // Force immediate retransmission by setting last_send to past
            it->second.last_send = std::chrono::steady_clock::time_point{};
        }

        std::vector<uint8_t> Reliability::build_ack_packet(uint16_t packet_id,
                                                           uint16_t packet_count,
                                                           const std::vector<bool>& received) {
            // Calculate bitset size: ceil(packet_count / 8)
            std::size_t bitset_bytes = (packet_count + 7) / 8;
            std::size_t total_size   = sizeof(ACKPacket) - 1 + bitset_bytes;  // -1 for the placeholder uint8_t

            std::vector<uint8_t> packet(total_size, 0);

            // Write header
            ACKPacket ack_header;
            ack_header.packet_id    = packet_id;
            ack_header.packet_count = packet_count;
            std::memcpy(packet.data(), &ack_header, sizeof(ACKPacket) - 1);

            // Write bitset
            uint8_t* bitset = packet.data() + sizeof(ACKPacket) - 1;
            for (std::size_t i = 0; i < received.size() && i < packet_count; ++i) {
                if (received[i]) {
                    bitset[i / 8] |= static_cast<uint8_t>(1u << (i % 8));
                }
            }

            return packet;
        }

        std::vector<uint8_t> Reliability::build_nack_packet(uint16_t packet_id,
                                                            uint16_t packet_count,
                                                            const std::vector<bool>& missing) {
            std::size_t bitset_bytes = (packet_count + 7) / 8;
            std::size_t total_size   = sizeof(NACKPacket) - 1 + bitset_bytes;

            std::vector<uint8_t> packet(total_size, 0);

            NACKPacket nack_header;
            nack_header.packet_id    = packet_id;
            nack_header.packet_count = packet_count;
            std::memcpy(packet.data(), &nack_header, sizeof(NACKPacket) - 1);

            uint8_t* bitset = packet.data() + sizeof(NACKPacket) - 1;
            for (std::size_t i = 0; i < missing.size() && i < packet_count; ++i) {
                if (missing[i]) {
                    bitset[i / 8] |= static_cast<uint8_t>(1u << (i % 8));
                }
            }

            return packet;
        }

        std::vector<Reliability::RetransmitRequest> Reliability::check_retransmissions(uint16_t packet_mtu) {
            auto now = std::chrono::steady_clock::now();
            std::vector<RetransmitRequest> retransmissions;
            std::vector<TrackingKey> expired;

            const std::lock_guard<std::mutex> lock(tracking_mutex);

            for (auto& [key, tp] : tracked_packets) {
                // Get the timeout for this peer
                std::chrono::steady_clock::duration rto;
                {
                    const std::lock_guard<std::mutex> rtt_lock(rtt_mutex);
                    rto = rtt_estimators[tp.target].timeout();
                }

                // Check if it's time to retransmit
                if (now - tp.last_send < rto) {
                    continue;
                }

                // Check if we've exceeded max retransmits
                if (max_retransmits > 0 && tp.retransmit_count >= max_retransmits) {
                    expired.push_back(key);
                    continue;
                }

                // Retransmit unacked fragments
                for (uint16_t i = 0; i < tp.packet_count; ++i) {
                    if (!tp.acked[i]) {
                        RetransmitRequest req;
                        req.target       = tp.target;
                        req.packet_id    = tp.packet_id;
                        req.packet_no    = i;
                        req.packet_count = tp.packet_count;
                        req.flags        = tp.flags;
                        req.hash         = tp.hash;

                        // Extract the fragment data
                        std::size_t offset = static_cast<std::size_t>(i) * packet_mtu;
                        std::size_t length = std::min(static_cast<std::size_t>(packet_mtu), tp.payload.size() - offset);
                        req.data.assign(tp.payload.begin() + offset, tp.payload.begin() + offset + length);

                        retransmissions.push_back(std::move(req));
                    }
                }

                tp.last_send = now;
                tp.retransmit_count++;
            }

            // Remove expired entries
            for (const auto& key : expired) {
                tracked_packets.erase(key);
            }

            return retransmissions;
        }

        void Reliability::remove_peer(const sock_t& target) {
            {
                const std::lock_guard<std::mutex> lock(tracking_mutex);
                for (auto it = tracked_packets.begin(); it != tracked_packets.end();) {
                    if (it->first.first == target) {
                        it = tracked_packets.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
            }
            {
                const std::lock_guard<std::mutex> lock(rtt_mutex);
                rtt_estimators.erase(target);
            }
        }

        RTTEstimator& Reliability::get_rtt(const sock_t& target) {
            const std::lock_guard<std::mutex> lock(rtt_mutex);
            return rtt_estimators[target];
        }

    }  // namespace network
}  // namespace NUClear
