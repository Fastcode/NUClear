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
#ifndef NUCLEAR_EXTENSION_NETWORK_PACKET_DEDUPLICATOR_HPP
#define NUCLEAR_EXTENSION_NETWORK_PACKET_DEDUPLICATOR_HPP

#include <bitset>
#include <cstdint>

namespace NUClear {
namespace extension {
    namespace network {

        /**
         * A class that implements a sliding window bitset for packet deduplication.
         * Maintains a 256-bit window of recently seen packet IDs, sliding forward as new packets are added.
         */
        class PacketDeduplicator {
        public:
            /**
             * Check if a packet ID has been seen recently
             *
             * @param packet_id The packet ID to check
             *
             * @return true if the packet has been seen recently, false otherwise
             */
            bool is_duplicate(uint16_t packet_id) const;

            /**
             * Add a packet ID to the window
             *
             * @param packet_id The packet ID to add
             */
            void add_packet(uint16_t packet_id);

        private:
            /// Whether we've seen any packets yet
            bool initialized{false};
            /// The newest packet ID we've seen
            uint16_t newest_seen{0};
            /// The 256-bit window of seen packets (newest at 0, older at higher indices)
            std::bitset<256> window;
        };

    }  // namespace network
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORK_PACKET_DEDUPLICATOR_HPP
