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

#ifndef NUCLEAR_NETWORK_FRAGMENTATION_HPP
#define NUCLEAR_NETWORK_FRAGMENTATION_HPP

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <vector>

namespace NUClear {
namespace network {

    /**
     * Handles fragmentation of large messages into MTU-sized packets and reassembly on the receiving side.
     *
     * Responsibilities:
     * - Splitting a payload into fragments that fit within the packet MTU
     * - Reassembling received fragments back into complete messages
     * - Tracking incomplete assemblies with timeouts
     * - Enforcing maximum reassembly size limits to prevent memory bombs
     */
    class Fragmentation {
    public:
        /// Result of a completed reassembly
        struct AssembledPacket {
            uint16_t packet_id{0};
            uint64_t hash{0};
            uint8_t flags{0};
            std::vector<uint8_t> payload;
        };

        /// A single fragment ready to be sent
        struct Fragment {
            uint16_t packet_id{0};
            uint16_t packet_no{0};
            uint16_t packet_count{0};
            uint8_t flags{0};
            uint64_t hash{0};
            std::vector<uint8_t> data;
        };

        /**
         * Construct the fragmentation module.
         *
         * @param packet_mtu          Maximum payload bytes per fragment
         * @param max_assembly_size   Maximum total size of a reassembled message (0 = unlimited)
         * @param assembly_timeout    How long to keep an incomplete assembly before discarding.
         *                             Should match the peer timeout since if no fragments arrive within this period,
         *                             the peer will be considered dead and cleaned up anyway.
         */
        Fragmentation(uint16_t packet_mtu                                   = 1452,
                      std::size_t max_assembly_size                          = 64 * 1024 * 1024,  // 64 MB default
                      std::chrono::steady_clock::duration assembly_timeout   = std::chrono::seconds(2));

        /**
         * Fragment a message into MTU-sized pieces.
         *
         * @param packet_id  The unique ID for this packet group
         * @param hash       The message type hash
         * @param flags      Packet flags (e.g., RELIABLE)
         * @param payload    The full message payload
         *
         * @return Vector of fragments ready to be sent
         */
        std::vector<Fragment> fragment(uint16_t packet_id,
                                       uint64_t hash,
                                       uint8_t flags,
                                       const std::vector<uint8_t>& payload) const;

        /**
         * Submit a received fragment for reassembly.
         *
         * @param source      Opaque key identifying the sender (for per-peer assembly tracking)
         * @param packet_id   The packet group ID
         * @param packet_no   This fragment's index (0-based)
         * @param packet_count Total fragments in the group
         * @param hash        The message type hash
         * @param flags       Packet flags
         * @param data        The fragment payload
         *
         * @param out_packet  Filled with the assembled packet when reassembly completes
         * @param now         The current time (defaults to steady_clock::now())
         *
         * @return true if all fragments are now received and @p out_packet is valid, false otherwise
         */
        bool submit_fragment(uint64_t source_key,
                             uint16_t packet_id,
                             uint16_t packet_no,
                             uint16_t packet_count,
                             uint64_t hash,
                             uint8_t flags,
                             const uint8_t* data,
                             std::size_t data_length,
                             AssembledPacket& out_packet,
                             std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

        /**
         * Clean up assemblies that have timed out.
         *
         * @param now The current time (defaults to steady_clock::now())
         *
         * @return Number of assemblies that were discarded
         */
        std::size_t cleanup_expired(
            std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());

        /**
         * Get the packet MTU (max payload per fragment).
         */
        uint16_t get_packet_mtu() const {
            return packet_mtu;
        }

    private:
        /// Key for an in-progress assembly: (source_key, packet_id)
        using AssemblyKey = std::pair<uint64_t, uint16_t>;

        /// State for an in-progress reassembly
        struct Assembly {
            uint64_t hash{0};
            uint8_t flags{0};
            uint16_t packet_count{0};
            std::chrono::steady_clock::time_point last_update;
            std::map<uint16_t, std::vector<uint8_t>> fragments;
        };

        uint16_t packet_mtu;
        std::size_t max_assembly_size;
        std::chrono::steady_clock::duration assembly_timeout;

        mutable std::mutex assembly_mutex;
        std::map<AssemblyKey, Assembly> assemblies;
    };

}  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_FRAGMENTATION_HPP
