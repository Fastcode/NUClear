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

#include "nuclear_net/wire_protocol.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>

using namespace NUClear::network;

TEST_CASE("Wire protocol structs have correct packed sizes", "[nuclear_net][wire_protocol]") {
    // PacketHeader: magic(3) + version(1) + type(1) = 5
    REQUIRE(sizeof(PacketHeader) == 5);

    // DataPacket: header(5) + packet_id(2) + packet_no(2) + packet_count(2) + flags(1) + hash(8) + data(1) = 21
    REQUIRE(sizeof(DataPacket) == 21);

    // ACKPacket: header(5) + packet_id(2) + packet_count(2) + packets(1) = 10
    REQUIRE(sizeof(ACKPacket) == 10);

    // NACKPacket: header(5) + packet_id(2) + packet_count(2) + packets(1) = 10
    REQUIRE(sizeof(NACKPacket) == 10);

    // LeavePacket: just the header = 5
    REQUIRE(sizeof(LeavePacket) == 5);
}

TEST_CASE("Wire protocol header is laid out at expected byte offsets", "[nuclear_net][wire_protocol]") {
    DataPacket pkt;
    pkt.packet_id    = 0x0102;
    pkt.packet_no    = 0x0304;
    pkt.packet_count = 0x0506;
    pkt.flags        = 0x07;
    pkt.hash         = 0x08090A0B0C0D0E0F;

    const auto* raw = reinterpret_cast<const uint8_t*>(&pkt);

    // Magic bytes at offset 0-2
    REQUIRE(raw[0] == 0xE2);
    REQUIRE(raw[1] == 0x98);
    REQUIRE(raw[2] == 0xA2);

    // Version at offset 3
    REQUIRE(raw[3] == PROTOCOL_VERSION);

    // Type at offset 4
    REQUIRE(raw[4] == DATA);

    // packet_id at offset 5-6
    uint16_t pid;
    std::memcpy(&pid, raw + 5, 2);
    REQUIRE(pid == 0x0102);

    // packet_no at offset 7-8
    uint16_t pno;
    std::memcpy(&pno, raw + 7, 2);
    REQUIRE(pno == 0x0304);

    // packet_count at offset 9-10
    uint16_t pcnt;
    std::memcpy(&pcnt, raw + 9, 2);
    REQUIRE(pcnt == 0x0506);

    // flags at offset 11
    REQUIRE(raw[11] == 0x07);

    // hash at offset 12-19
    uint64_t h;
    std::memcpy(&h, raw + 12, 8);
    REQUIRE(h == 0x08090A0B0C0D0E0F);
}

TEST_CASE("validate_header accepts valid packets", "[nuclear_net][wire_protocol]") {
    DataPacket pkt;
    const auto* raw = reinterpret_cast<const uint8_t*>(&pkt);

    REQUIRE(validate_header(raw, sizeof(DataPacket)));
}

TEST_CASE("validate_header rejects packet too short", "[nuclear_net][wire_protocol]") {
    uint8_t data[4] = {0xE2, 0x98, 0xA2, 0x03};
    REQUIRE_FALSE(validate_header(data, 4));
}

TEST_CASE("validate_header rejects wrong magic bytes", "[nuclear_net][wire_protocol]") {
    uint8_t data[5] = {0x00, 0x00, 0x00, 0x03, 0x01};
    REQUIRE_FALSE(validate_header(data, 5));
}

TEST_CASE("validate_header rejects wrong protocol version", "[nuclear_net][wire_protocol]") {
    uint8_t data[5] = {0xE2, 0x98, 0xA2, 0x99, 0x01};  // Version 0x99
    REQUIRE_FALSE(validate_header(data, 5));
}
