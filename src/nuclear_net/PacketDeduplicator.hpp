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

#ifndef NUCLEAR_NETWORK_PACKET_DEDUPLICATOR_HPP
#define NUCLEAR_NETWORK_PACKET_DEDUPLICATOR_HPP

#include <bitset>
#include <cstdint>

namespace NUClear {
namespace network {

    /**
     * Sliding window bitset for packet deduplication.
     *
     * Maintains a 256-bit window of recently seen packet IDs.
     * The window slides forward as newer packets are added.
     * Packets older than 256 IDs behind the newest are considered duplicates.
     *
     * Uses uint16_t packet IDs with unsigned modular arithmetic for wrap-around handling.
     */
    class PacketDeduplicator {
    public:
        /// Window size in bits
        static constexpr uint16_t WINDOW_SIZE = 256;

        /**
         * Check if a packet ID has been seen recently.
         *
         * @param packet_id The packet ID to check
         *
         * @return true if the packet has been seen recently (is a duplicate), false otherwise
         */
        bool is_duplicate(uint16_t packet_id) const;

        /**
         * Add a packet ID to the window, marking it as seen.
         * If the packet_id is newer than the current window, the window slides forward.
         *
         * @param packet_id The packet ID to add
         */
        void add_packet(uint16_t packet_id);

    private:
        /// Whether we've seen any packets yet
        bool initialized{false};
        /// The newest packet ID we've seen
        uint16_t newest_seen{0};
        /// The 256-bit window of seen packets (bit 0 = newest_seen, higher indices = older)
        std::bitset<WINDOW_SIZE> window;
    };

}  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_PACKET_DEDUPLICATOR_HPP
