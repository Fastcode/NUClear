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

#include "nuclearnet/Reliability.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>

#include "util/platform.hpp"

using NUClear::network::Reliability;
using NUClear::util::network::sock_t;

namespace {
sock_t make_addr(uint32_t ip, uint16_t port) {
    sock_t addr{};
    addr.ipv4.sin_family      = AF_INET;
    addr.ipv4.sin_port        = htons(port);
    addr.ipv4.sin_addr.s_addr = htonl(ip);
    return addr;
}
}  // namespace

SCENARIO("Reliability tracks sent packet and requests retransmission after timeout", "[nuclearnet][reliability]") {
    Reliability rel;

    sock_t target = make_addr(0x0A000001, 5000);
    std::vector<uint8_t> payload(200, 0xAB);

    // Track a 2-fragment packet at time T
    auto t = std::chrono::steady_clock::now();
    rel.track_packet(target, 1, 2, 0x1234, 0x01, payload.data(), payload.size(), t);

    // Immediately, no retransmissions (timeout not elapsed)
    auto retransmissions = rel.check_retransmissions(100, t);
    REQUIRE(retransmissions.empty());

    // Inject an RTT measurement to reduce the timeout to min_rto (100ms)
    rel.get_rtt(target).measure(std::chrono::milliseconds(10));

    // Check at T+150ms (past min_rto of 100ms) — should retransmit
    retransmissions = rel.check_retransmissions(100, t + std::chrono::milliseconds(150));
    REQUIRE(retransmissions.size() == 2);  // Both fragments unacked
    REQUIRE(retransmissions[0].packet_no == 0);
    REQUIRE(retransmissions[1].packet_no == 1);
    REQUIRE(retransmissions[0].data.size() == 100);
    REQUIRE(retransmissions[1].data.size() == 100);
}

SCENARIO("Reliability stops retransmitting ACKed fragments", "[nuclearnet][reliability]") {
    Reliability rel;

    sock_t target = make_addr(0x0A000001, 5000);
    std::vector<uint8_t> payload(200, 0xCC);

    auto t = std::chrono::steady_clock::now();
    rel.track_packet(target, 1, 2, 0x1234, 0x01, payload.data(), payload.size(), t);

    // ACK fragment 0 (bitset: bit 0 set)
    uint8_t ack_bits = 0x01;  // fragment 0 received
    rel.process_ack(target, 1, 2, &ack_bits, 1, t + std::chrono::milliseconds(50));

    // Inject short RTT and check past min_rto
    rel.get_rtt(target).measure(std::chrono::milliseconds(5));

    auto retransmissions = rel.check_retransmissions(100, t + std::chrono::milliseconds(200));
    // Only fragment 1 should be retransmitted (fragment 0 was ACKed)
    REQUIRE(retransmissions.size() == 1);
    REQUIRE(retransmissions[0].packet_no == 1);
}

SCENARIO("Reliability removes tracked packet when all fragments ACKed", "[nuclearnet][reliability]") {
    Reliability rel;

    sock_t target = make_addr(0x0A000001, 5000);
    std::vector<uint8_t> payload(100, 0xDD);

    auto t = std::chrono::steady_clock::now();
    rel.track_packet(target, 5, 1, 0x5678, 0x01, payload.data(), payload.size(), t);

    // ACK all fragments
    uint8_t ack_bits = 0x01;  // fragment 0 received (only 1 fragment total)
    rel.process_ack(target, 5, 1, &ack_bits, 1, t + std::chrono::milliseconds(50));

    // Inject short RTT and check well past min_rto — nothing to retransmit
    rel.get_rtt(target).measure(std::chrono::milliseconds(5));

    auto retransmissions = rel.check_retransmissions(100, t + std::chrono::milliseconds(200));
    REQUIRE(retransmissions.empty());
}

SCENARIO("Reliability retransmits indefinitely until peer is removed", "[nuclearnet][reliability]") {
    Reliability rel;

    sock_t target = make_addr(0x0A000001, 5000);
    std::vector<uint8_t> payload(50, 0xEE);

    auto t = std::chrono::steady_clock::now();
    rel.track_packet(target, 1, 1, 0x1234, 0x01, payload.data(), payload.size(), t);

    // Inject short RTT
    rel.get_rtt(target).measure(std::chrono::milliseconds(10));

    // First retransmission (T+150ms, past min_rto of 100ms)
    auto r1 = rel.check_retransmissions(100, t + std::chrono::milliseconds(150));
    REQUIRE(r1.size() == 1);

    // Second retransmission (T+300ms, 150ms since last_send was updated to T+150ms)
    auto r2 = rel.check_retransmissions(100, t + std::chrono::milliseconds(300));
    REQUIRE(r2.size() == 1);

    // Third retransmission still works — no limit
    auto r3 = rel.check_retransmissions(100, t + std::chrono::milliseconds(450));
    REQUIRE(r3.size() == 1);

    // Removing the peer cleans up all tracked packets
    rel.remove_peer(target);
    auto r4 = rel.check_retransmissions(100, t + std::chrono::milliseconds(600));
    REQUIRE(r4.empty());
}

SCENARIO("Reliability build_ack_packet encodes bitset correctly", "[nuclearnet][reliability]") {
    std::vector<bool> received = {true, false, true, true, false, false, false, true};  // 0b10001101 = 0x8D
    auto ack = Reliability::build_ack_packet(42, 8, received);

    // The packet should contain a header + 1 byte of bitset
    // Verify the bitset byte
    REQUIRE(ack.size() > 0);
    // The last byte(s) should be the bitset
    uint8_t bitset_byte = ack.back();
    REQUIRE(bitset_byte == 0x8D);
}

SCENARIO("Reliability remove_peer removes all tracked state", "[nuclearnet][reliability]") {
    Reliability rel;

    sock_t target = make_addr(0x0A000001, 5000);
    std::vector<uint8_t> payload(100, 0xFF);

    auto t = std::chrono::steady_clock::now();
    rel.track_packet(target, 1, 1, 0x1234, 0x01, payload.data(), payload.size(), t);
    rel.get_rtt(target).measure(std::chrono::milliseconds(50));

    rel.remove_peer(target);

    // After removing, no retransmissions should occur even well past RTO
    auto retransmissions = rel.check_retransmissions(100, t + std::chrono::milliseconds(500));
    REQUIRE(retransmissions.empty());
}
