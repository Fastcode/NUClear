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
namespace network {

    bool PacketDeduplicator::is_duplicate(uint16_t packet_id) const {
        // If we haven't seen any packets yet, nothing is a duplicate
        if (!initialized) {
            return false;
        }

        // Check how far ahead of newest_seen this packet is
        uint16_t forward_distance = packet_id - newest_seen;

        // If it's ahead of newest_seen (newer), it can't be a duplicate
        if (forward_distance != 0 && forward_distance < 0x8000U) {
            return false;
        }

        // It's at or behind newest_seen — check the distance backward
        uint16_t distance = newest_seen - packet_id;

        // If distance >= WINDOW_SIZE, it's too old — treat as duplicate (already processed or lost)
        if (distance >= WINDOW_SIZE) {
            return true;
        }

        // Check the bit in our window
        return window.test(distance);
    }

    void PacketDeduplicator::add_packet(uint16_t packet_id) {
        if (!initialized) {
            // First packet ever — initialize the window
            initialized = true;
            newest_seen = packet_id;
            window.reset();
            window.set(0);  // Mark current packet as seen
            return;
        }

        // Calculate how far ahead of newest_seen this packet is (signed interpretation of unsigned diff)
        uint16_t forward_distance = packet_id - newest_seen;

        // If the high bit is set, it's actually behind us (wrapped subtraction gave a large positive number)
        // We use half the uint16_t range as the threshold for "ahead" vs "behind"
        if (forward_distance == 0) {
            // Same as newest — just make sure the bit is set
            window.set(0);
        }
        else if (forward_distance < 0x8000U) {
            // This packet is NEWER than our current newest
            // Slide the window forward by forward_distance positions
            if (forward_distance >= WINDOW_SIZE) {
                // The new packet is so far ahead that the entire window is invalidated
                window.reset();
            }
            else {
                window <<= forward_distance;
            }
            newest_seen = packet_id;
            window.set(0);  // Mark the new packet as seen
        }
        else {
            // This packet is OLDER than our current newest (behind us)
            uint16_t distance = newest_seen - packet_id;
            if (distance < WINDOW_SIZE) {
                window.set(distance);  // Mark it as seen in the appropriate position
            }
            // If it's too old (distance >= WINDOW_SIZE), we just ignore it
        }
    }

}  // namespace network
}  // namespace NUClear
