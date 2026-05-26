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

#include "nuclearnet/Fragmentation.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <numeric>
#include <vector>

using NUClear::network::Fragmentation;

SCENARIO("Fragmentation splits large payload into MTU-sized fragments", "[nuclearnet][fragmentation]") {
    Fragmentation frag(100);  // 100-byte MTU

    // 250 bytes of data → should produce 3 fragments (100 + 100 + 50)
    std::vector<uint8_t> payload(250);
    std::iota(payload.begin(), payload.end(), uint8_t(0));

    auto fragments = frag.fragment(1, 0xDEADBEEF, 0, payload);

    REQUIRE(fragments.size() == 3);
    REQUIRE(fragments[0].packet_no == 0);
    REQUIRE(fragments[1].packet_no == 1);
    REQUIRE(fragments[2].packet_no == 2);

    REQUIRE(fragments[0].packet_count == 3);
    REQUIRE(fragments[1].packet_count == 3);
    REQUIRE(fragments[2].packet_count == 3);

    REQUIRE(fragments[0].data.size() == 100);
    REQUIRE(fragments[1].data.size() == 100);
    REQUIRE(fragments[2].data.size() == 50);

    // Verify data integrity
    REQUIRE(fragments[0].data[0] == 0);
    REQUIRE(fragments[0].data[99] == 99);
    REQUIRE(fragments[1].data[0] == 100);
    REQUIRE(fragments[2].data[0] == 200);
    REQUIRE(fragments[2].data[49] == 249);
}

SCENARIO("Fragmentation produces single fragment for small payload", "[nuclearnet][fragmentation]") {
    Fragmentation frag(1452);

    std::vector<uint8_t> payload(100, 0xAB);
    auto fragments = frag.fragment(5, 0x12345678, 0x01, payload);

    REQUIRE(fragments.size() == 1);
    REQUIRE(fragments[0].packet_no == 0);
    REQUIRE(fragments[0].packet_count == 1);
    REQUIRE(fragments[0].data.size() == 100);
    REQUIRE(fragments[0].hash == 0x12345678);
    REQUIRE(fragments[0].flags == 0x01);
}

SCENARIO("Fragmentation produces single fragment for empty payload", "[nuclearnet][fragmentation]") {
    Fragmentation frag(1452);

    std::vector<uint8_t> payload;
    auto fragments = frag.fragment(0, 0x11111111, 0, payload);

    REQUIRE(fragments.size() == 1);
    REQUIRE(fragments[0].packet_count == 1);
    REQUIRE(fragments[0].data.empty());
}

SCENARIO("Fragmentation reassembles fragments into original payload", "[nuclearnet][fragmentation]") {
    Fragmentation frag(100);

    // Create a payload and fragment it
    std::vector<uint8_t> payload(250);
    std::iota(payload.begin(), payload.end(), uint8_t(0));

    auto fragments = frag.fragment(1, 0xDEADBEEF, 0, payload);

    // Submit fragments in order
    Fragmentation::AssembledPacket result;
    bool complete = false;
    for (const auto& f : fragments) {
        complete = frag.submit_fragment(99,
                                        f.packet_id,
                                        f.packet_no,
                                        f.packet_count,
                                        f.hash,
                                        f.flags,
                                        f.data.data(),
                                        f.data.size(),
                                        result);
    }

    REQUIRE(complete);
    REQUIRE(result.payload == payload);
    REQUIRE(result.hash == 0xDEADBEEF);
    REQUIRE(result.packet_id == 1);
}

SCENARIO("Fragmentation reassembles out-of-order fragments", "[nuclearnet][fragmentation]") {
    Fragmentation frag(100);

    std::vector<uint8_t> payload(250);
    std::iota(payload.begin(), payload.end(), uint8_t(0));

    auto fragments = frag.fragment(7, 0xCAFEBABE, 0x01, payload);

    // Submit in reverse order
    Fragmentation::AssembledPacket result;
    REQUIRE_FALSE(frag.submit_fragment(1,
                                       7,
                                       2,
                                       3,
                                       0xCAFEBABE,
                                       0x01,
                                       fragments[2].data.data(),
                                       fragments[2].data.size(),
                                       result));
    REQUIRE_FALSE(frag.submit_fragment(1,
                                       7,
                                       0,
                                       3,
                                       0xCAFEBABE,
                                       0x01,
                                       fragments[0].data.data(),
                                       fragments[0].data.size(),
                                       result));

    // Last fragment completes the assembly
    bool complete = frag.submit_fragment(1,
                                         7,
                                         1,
                                         3,
                                         0xCAFEBABE,
                                         0x01,
                                         fragments[1].data.data(),
                                         fragments[1].data.size(),
                                         result);

    REQUIRE(complete);
    REQUIRE(result.payload == payload);
}

SCENARIO("Fragmentation rejects oversized assemblies", "[nuclearnet][fragmentation]") {
    // Allow only 200 bytes total
    Fragmentation frag(100, 200);

    // Try to submit a fragment that implies a total size > 200 bytes
    // 3 fragments × 100 byte MTU = 300 bytes projected > 200 byte limit
    uint8_t data[100] = {};
    Fragmentation::AssembledPacket result;
    bool complete = frag.submit_fragment(1, 1, 0, 3, 0x1234, 0, data, 100, result);
    REQUIRE_FALSE(complete);
}

SCENARIO("Fragmentation rejects invalid fragment indices", "[nuclearnet][fragmentation]") {
    Fragmentation frag(100);

    uint8_t data[50] = {};

    // packet_no >= packet_count is invalid
    Fragmentation::AssembledPacket result;
    bool complete = frag.submit_fragment(1, 1, 5, 3, 0x1234, 0, data, 50, result);
    REQUIRE_FALSE(complete);

    // packet_count == 0 is invalid
    complete = frag.submit_fragment(1, 1, 0, 0, 0x1234, 0, data, 50, result);
    REQUIRE_FALSE(complete);
}

SCENARIO("Fragmentation cleanup_expired removes stale assemblies", "[nuclearnet][fragmentation]") {
    // Use a very short timeout for testing
    Fragmentation frag(100, 64 * 1024 * 1024, std::chrono::milliseconds(1));

    // Submit a partial assembly at time T
    auto t = std::chrono::steady_clock::now();
    uint8_t data[50] = {};
    Fragmentation::AssembledPacket result;
    frag.submit_fragment(1, 1, 0, 3, 0x1234, 0, data, 50, result, t);

    // Cleanup at T (not expired yet) — nothing removed
    std::size_t removed = frag.cleanup_expired(t);
    REQUIRE(removed == 0);

    // Cleanup at T+10ms (past 1ms timeout) — should remove it
    removed = frag.cleanup_expired(t + std::chrono::milliseconds(10));
    REQUIRE(removed == 1);

    // Second cleanup should find nothing
    removed = frag.cleanup_expired(t + std::chrono::milliseconds(20));
    REQUIRE(removed == 0);
}

SCENARIO("Fragmentation handles multiple independent assemblies", "[nuclearnet][fragmentation]") {
    Fragmentation frag(100);

    uint8_t data_a[100];
    uint8_t data_b[100];
    std::fill_n(data_a, 100, 0xAA);
    std::fill_n(data_b, 100, 0xBB);

    // Two different sources sending 2-fragment messages
    Fragmentation::AssembledPacket result1;
    Fragmentation::AssembledPacket result2;
    REQUIRE_FALSE(frag.submit_fragment(1, 10, 0, 2, 0x1111, 0, data_a, 100, result1));
    REQUIRE_FALSE(frag.submit_fragment(2, 10, 0, 2, 0x2222, 0, data_b, 100, result2));

    // Complete source 2's message
    REQUIRE(frag.submit_fragment(2, 10, 1, 2, 0x2222, 0, data_b, 100, result2));
    REQUIRE(result2.hash == 0x2222);
    REQUIRE(result2.payload.size() == 200);
    REQUIRE(result2.payload[0] == 0xBB);

    // Source 1 still incomplete
    // Complete it
    REQUIRE(frag.submit_fragment(1, 10, 1, 2, 0x1111, 0, data_a, 100, result1));
    REQUIRE(result1.hash == 0x1111);
    REQUIRE(result1.payload[0] == 0xAA);
}
