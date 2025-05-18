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
#include "PacketDeduplicator.hpp"

namespace NUClear {
namespace extension {
    namespace network {

        PacketDeduplicator::PacketDeduplicator() : newest_seen(0) {}

        bool PacketDeduplicator::is_duplicate(uint16_t packet_id) const {
            // If we haven't seen any packets yet, nothing is a duplicate
            if (!initialized) {
                return false;
            }

            // Calculate relative position in window using unsigned subtraction
            uint16_t relative_id = newest_seen - packet_id;

            // If the packet is too old or too new, it's not a duplicate
            if (relative_id >= 256) {
                return false;
            }

            return window[relative_id];
        }

        void PacketDeduplicator::add_packet(uint16_t packet_id) {
            // If this is our first packet, just set it as newest_seen
            if (!initialized) {
                newest_seen = packet_id;
                window[0]   = true;
                initialized = true;
                return;
            }

            // Calculate relative position in window using unsigned subtraction
            uint16_t relative_id = newest_seen - packet_id;

            // If the distance is more than half the range, the packet is newer than our newest_seen
            if (relative_id > 32768) {
                // Calculate how far to shift to make this packet our newest
                uint16_t shift_amount = packet_id - newest_seen;
                window <<= shift_amount;
                newest_seen = packet_id;
                window[0]   = true;
            }
            // Packet is recent enough to be counted
            else if (relative_id < 256) {
                window[relative_id] = true;
            }
        }

    }  // namespace network
}  // namespace extension
}  // namespace NUClear
