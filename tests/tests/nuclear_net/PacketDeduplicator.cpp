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

#include "nuclear_net/PacketDeduplicator.hpp"

#include <catch2/catch_test_macros.hpp>

using NUClear::network::PacketDeduplicator;

TEST_CASE("PacketDeduplicator rejects duplicate packet IDs", "[nuclear_net][deduplicator]") {
    PacketDeduplicator dedup;

    // First time seeing packet 42 — not a duplicate
    REQUIRE_FALSE(dedup.is_duplicate(42));

    // Add it to the seen set
    dedup.add_packet(42);

    // Now it should be detected as a duplicate
    REQUIRE(dedup.is_duplicate(42));
}

TEST_CASE("PacketDeduplicator accepts distinct packet IDs", "[nuclear_net][deduplicator]") {
    PacketDeduplicator dedup;

    dedup.add_packet(1);
    dedup.add_packet(2);
    dedup.add_packet(3);

    REQUIRE_FALSE(dedup.is_duplicate(4));
    REQUIRE_FALSE(dedup.is_duplicate(100));
    REQUIRE_FALSE(dedup.is_duplicate(255));
}

TEST_CASE("PacketDeduplicator sliding window advances and forgets old IDs", "[nuclear_net][deduplicator]") {
    PacketDeduplicator dedup;

    // Add packet 0
    dedup.add_packet(0);
    REQUIRE(dedup.is_duplicate(0));

    // Advance the window far enough that packet 0 falls outside the window (window size = 256)
    for (uint16_t i = 1; i <= 256; ++i) {
        dedup.add_packet(i);
    }

    // Packet 0 should now be outside the window
    // Since it's behind the window base, it should be treated as a duplicate (too old)
    REQUIRE(dedup.is_duplicate(0));
}

TEST_CASE("PacketDeduplicator handles sequential packet IDs", "[nuclear_net][deduplicator]") {
    PacketDeduplicator dedup;

    // Process packets 0-99 in order
    for (uint16_t i = 0; i < 100; ++i) {
        REQUIRE_FALSE(dedup.is_duplicate(i));
        dedup.add_packet(i);
    }

    // All should now be duplicates
    for (uint16_t i = 0; i < 100; ++i) {
        REQUIRE(dedup.is_duplicate(i));
    }
}

TEST_CASE("PacketDeduplicator handles uint16_t wraparound", "[nuclear_net][deduplicator]") {
    PacketDeduplicator dedup;

    // Start near the max value
    uint16_t start = 65500;
    for (uint16_t i = 0; i < 100; ++i) {
        uint16_t id = static_cast<uint16_t>(start + i);  // Will wrap around 65535 → 0
        REQUIRE_FALSE(dedup.is_duplicate(id));
        dedup.add_packet(id);
    }

    // IDs that wrapped around should be marked as seen
    REQUIRE(dedup.is_duplicate(65500));
    REQUIRE(dedup.is_duplicate(static_cast<uint16_t>(65535)));
    REQUIRE(dedup.is_duplicate(0));  // wrapped
    REQUIRE(dedup.is_duplicate(63));  // 65500 + 99 - 65536 = 63
}

TEST_CASE("PacketDeduplicator handles out-of-order within window", "[nuclear_net][deduplicator]") {
    PacketDeduplicator dedup;

    // Add packet 10 first (advances window)
    dedup.add_packet(10);

    // Packets 0-9 arrive late but are still within the 256-element window
    for (uint16_t i = 0; i < 10; ++i) {
        REQUIRE_FALSE(dedup.is_duplicate(i));
        dedup.add_packet(i);
    }

    // All should now be duplicates
    for (uint16_t i = 0; i <= 10; ++i) {
        REQUIRE(dedup.is_duplicate(i));
    }
}
