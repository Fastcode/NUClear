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

#ifndef NUCLEAR_NETWORK_RELIABILITY_HPP
#define NUCLEAR_NETWORK_RELIABILITY_HPP

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <vector>

#include "../util/network/sock_t.hpp"
#include "RTTEstimator.hpp"

namespace NUClear {
namespace network {

    /**
     * Handles reliable delivery via ACK/NACK tracking and retransmission.
     *
     * Responsibilities:
     * - Tracking which fragments have been ACKed for each reliable packet group
     * - Scheduling retransmissions based on RTT estimates
     * - Processing incoming ACK/NACK packets
     * - Providing per-peer RTT estimation
     * - Exponential backoff on repeated failures
     */
    class Reliability {
    public:
        using sock_t = util::network::sock_t;

        /// Information about a fragment that needs retransmitting
        struct RetransmitRequest {
            sock_t target;
            uint16_t packet_id;
            uint16_t packet_no;
            uint16_t packet_count;
            uint8_t flags;
            uint64_t hash;
            std::vector<uint8_t> data;
        };

        /**
         * Construct the reliability module.
         *
         * @param max_retransmits Maximum number of retransmissions before giving up (0 = unlimited)
         */
        explicit Reliability(uint16_t max_retransmits = 10);

        /**
         * Register a sent reliable packet group for tracking.
         *
         * @param target       The peer we sent to
         * @param packet_id    The packet group ID
         * @param packet_count Total fragments in the group
         * @param hash         Message type hash
         * @param flags        Packet flags
         * @param payload      Pointer to the full original payload (copied internally for retransmission)
         * @param payload_len  Length of the payload in bytes
         */
        void track_packet(const sock_t& target,
                          uint16_t packet_id,
                          uint16_t packet_count,
                          uint64_t hash,
                          uint8_t flags,
                          const uint8_t* payload,
                          std::size_t payload_len);

        /**
         * Process a received ACK packet.
         *
         * @param source       Who sent the ACK
         * @param packet_id    The packet group being acknowledged
         * @param packet_count Total fragments in the group
         * @param ack_bitset   Bitset of received fragments (1 bit per fragment, LSB first)
         * @param bitset_size  Size of the ack_bitset in bytes
         */
        void process_ack(const sock_t& source,
                         uint16_t packet_id,
                         uint16_t packet_count,
                         const uint8_t* ack_bitset,
                         std::size_t bitset_size);

        /**
         * Process a received NACK packet.
         *
         * @param source       Who sent the NACK
         * @param packet_id    The packet group with missing fragments
         * @param packet_count Total fragments in the group
         * @param nack_bitset  Bitset of MISSING fragments (1 = missing)
         * @param bitset_size  Size of the nack_bitset in bytes
         */
        void process_nack(const sock_t& source,
                          uint16_t packet_id,
                          uint16_t packet_count,
                          const uint8_t* nack_bitset,
                          std::size_t bitset_size);

        /**
         * Build an ACK packet payload (excluding header) for a packet group.
         *
         * @param packet_id    The packet group to acknowledge
         * @param packet_count Total fragments
         * @param received     Bitset of which fragments have been received
         *
         * @return Serialized ACK packet bytes (complete packet including header)
         */
        static std::vector<uint8_t> build_ack_packet(uint16_t packet_id,
                                                     uint16_t packet_count,
                                                     const std::vector<bool>& received);

        /**
         * Build a NACK packet for missing fragments.
         *
         * @param packet_id    The packet group
         * @param packet_count Total fragments
         * @param missing      Bitset of which fragments are missing (true = missing)
         *
         * @return Serialized NACK packet bytes (complete packet including header)
         */
        static std::vector<uint8_t> build_nack_packet(uint16_t packet_id,
                                                      uint16_t packet_count,
                                                      const std::vector<bool>& missing);

        /**
         * Check for packets that need retransmission and return them.
         *
         * @param packet_mtu The MTU to use for fragmenting retransmissions
         *
         * @return List of fragments that need to be retransmitted
         */
        std::vector<RetransmitRequest> check_retransmissions(uint16_t packet_mtu);

        /**
         * Remove all tracking for a given peer (e.g., on disconnect).
         *
         * @param target The peer to remove
         */
        void remove_peer(const sock_t& target);

        /**
         * Get the RTT estimator for a specific peer.
         *
         * @param target The peer address
         * @return Reference to the RTT estimator (creates one if it doesn't exist)
         */
        RTTEstimator& get_rtt(const sock_t& target);

    private:
        /// State for a tracked reliable packet group
        struct TrackedPacket {
            sock_t target;
            uint16_t packet_id;
            uint16_t packet_count;
            uint64_t hash;
            uint8_t flags;
            std::vector<uint8_t> payload;
            std::vector<bool> acked;  ///< Which fragments have been ACKed
            std::chrono::steady_clock::time_point last_send;
            uint16_t retransmit_count{0};
        };

        /// Key for tracked packets: (target, packet_id)
        using TrackingKey = std::pair<sock_t, uint16_t>;

        uint16_t max_retransmits;

        mutable std::mutex tracking_mutex;
        std::map<TrackingKey, TrackedPacket> tracked_packets;

        mutable std::mutex rtt_mutex;
        std::map<sock_t, RTTEstimator> rtt_estimators;
    };

}  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_RELIABILITY_HPP
