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

#include "nuclearnet/NUClearNet.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nuclearnet/Discovery.hpp"
#include "nuclearnet/Reliability.hpp"
#include "nuclearnet/wire_protocol.hpp"
#include "test_util/has_multicast.hpp"
#include "util/network/sock_t.hpp"
#include "util/platform.hpp"

using NUClear::network::NUClearNet;
using NUClear::network::NetworkConfig;
using NUClear::network::PeerInfo;
using NUClear::util::network::sock_t;

using NUClear::network::Discovery;
using NUClear::network::Reliability;
using NUClear::network::ACK;
using NUClear::network::ANNOUNCE;
using NUClear::network::CON_ACK;
using NUClear::network::CONNECT;
using NUClear::network::DATA;
using NUClear::network::DataPacket;
using NUClear::network::PacketHeader;
using NUClear::network::PROTOCOL_VERSION;
using NUClear::network::SYN;
using NUClear::network::validate_header;

namespace {

/// Create a fake IPv4 address for use as a peer source
sock_t make_addr(uint32_t ip, uint16_t port) {
    sock_t addr{};
    addr.ipv4.sin_family      = AF_INET;
    addr.ipv4.sin_port        = htons(port);
    addr.ipv4.sin_addr.s_addr = htonl(ip);
    return addr;
}

/// Build a valid announce packet for a peer with the given name and subscriptions
std::vector<uint8_t> build_announce(const std::string& name, const std::vector<uint64_t>& subs) {
    return Discovery::build_announce_packet(name, subs);
}

/// Build a valid DATA packet containing the given payload
std::vector<uint8_t> build_data_packet(uint16_t packet_id,
                                       uint16_t packet_no,
                                       uint16_t packet_count,
                                       uint64_t hash,
                                       uint8_t flags,
                                       const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> pkt(sizeof(DataPacket) - 1 + payload.size());
    auto* header         = reinterpret_cast<DataPacket*>(pkt.data());  // NOLINT
    header->packet_id    = packet_id;
    header->packet_no    = packet_no;
    header->packet_count = packet_count;
    header->hash         = hash;
    header->flags        = flags;
    std::memcpy(pkt.data() + sizeof(DataPacket) - 1, payload.data(), payload.size());
    return pkt;
}

/// Build a valid ACK packet
std::vector<uint8_t> build_ack_packet(uint16_t packet_id, uint16_t packet_count, const std::vector<bool>& received) {
    return Reliability::build_ack_packet(packet_id, packet_count, received);
}

/// Build a CONNECT packet with given flags
std::vector<uint8_t> build_connect(uint8_t flags) {
    return Discovery::build_connect_packet(flags);
}

/// Establish a peer as fully connected (announce heard + handshake confirmed)
/// by running the full handshake sequence
void establish_peer(NUClearNet& net, const sock_t& peer_addr, const std::string& peer_name) {
    // Peer sends announce → we learn about them
    auto announce_pkt = build_announce(peer_name, {});
    net.process_packet(peer_addr, announce_pkt.data(), announce_pkt.size());

    // Peer sends CONNECT(SYN) → we respond with SYN+ACK
    auto syn = build_connect(SYN);
    net.process_connect_packet(peer_addr, syn.data(), syn.size());

    // Peer sends CONNECT(SYN+ACK) → we respond with ACK, completing handshake
    auto syn_ack = build_connect(SYN | CON_ACK);
    net.process_connect_packet(peer_addr, syn_ack.data(), syn_ack.size());

    // Peer sends CONNECT(ACK) → confirms connection
    auto ack = build_connect(CON_ACK);
    net.process_connect_packet(peer_addr, ack.data(), ack.size());
}

/// Set up a NUClearNet instance with loopback config for testing
std::unique_ptr<NUClearNet> make_test_net(const std::string& name = "TestNode") {
    auto net = std::make_unique<NUClearNet>();
    if (test_util::has_ipv4_multicast()) {
        NetworkConfig cfg;
        cfg.name = name;
        net->reset(cfg);
    }
    return net;
}

}  // namespace


// ===========================================================================
// validate_header behavioral tests
// ===========================================================================

SCENARIO("validate_header rejects wrong magic bytes", "[nuclearnet][process_packet]") {
    const std::array<uint8_t, 5> data = {0x00, 0x98, 0xA2, PROTOCOL_VERSION, ANNOUNCE};
    REQUIRE_FALSE(validate_header(data.data(), data.size()));
}

SCENARIO("validate_header rejects wrong protocol version", "[nuclearnet][process_packet]") {
    const std::array<uint8_t, 5> data = {0xE2, 0x98, 0xA2, 0xFF, ANNOUNCE};
    REQUIRE_FALSE(validate_header(data.data(), data.size()));
}

SCENARIO("validate_header rejects invalid packet type", "[nuclearnet][process_packet]") {
    const std::array<uint8_t, 5> data = {0xE2, 0x98, 0xA2, PROTOCOL_VERSION, 0xFF};
    REQUIRE_FALSE(validate_header(data.data(), data.size()));
}

SCENARIO("validate_header accepts all valid packet types", "[nuclearnet][process_packet]") {
    for (uint8_t type = ANNOUNCE; type <= CONNECT; ++type) {
        const std::array<uint8_t, 5> data = {0xE2, 0x98, 0xA2, PROTOCOL_VERSION, type};
        REQUIRE(validate_header(data.data(), data.size()));
    }
}


// ===========================================================================
// process_packet dispatch tests
// ===========================================================================

SCENARIO("process_packet discards packets with invalid headers", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    bool received = false;
    net->set_packet_callback([&](const sock_t&, const std::string&, uint64_t, bool, std::vector<uint8_t>) {
        received = true;
    });

    // Garbage data
    const std::array<uint8_t, 10> garbage = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    net->process_packet(peer, garbage.data(), garbage.size());

    REQUIRE_FALSE(received);
}

SCENARIO("process_packet dispatches LEAVE to discovery", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    bool left = false;
    net->set_leave_callback([&](const PeerInfo& info) {
        left = (info.address == peer);
    });

    // First establish the peer
    establish_peer(*net, peer, "Peer1");

    // Now send LEAVE
    auto leave_pkt = Discovery::build_leave_packet();
    net->process_packet(peer, leave_pkt.data(), leave_pkt.size());

    REQUIRE(left);
}


// ===========================================================================
// process_connect_packet behavioral tests
// ===========================================================================

SCENARIO("process_connect_packet ignores packets that are too short", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    // Establish announce so we have a peer entry
    auto announce_pkt = build_announce("Peer1", {});
    net->process_packet(peer, announce_pkt.data(), announce_pkt.size());

    bool joined = false;
    net->set_join_callback([&](const PeerInfo&) { joined = true; });

    // Send a packet that has a valid header but is too short for ConnectPacket
    PacketHeader hdr(CONNECT);
    net->process_connect_packet(peer, reinterpret_cast<const uint8_t*>(&hdr), sizeof(PacketHeader));

    // Should not have completed a join since the packet was too short
    REQUIRE_FALSE(joined);
}


// ===========================================================================
// process_data_packet behavioral tests
// ===========================================================================

SCENARIO("process_data_packet rejects data from unconnected peers", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    bool received = false;
    net->set_packet_callback([&](const sock_t&, const std::string&, uint64_t, bool, std::vector<uint8_t>) {
        received = true;
    });

    // Add subscription so routing would allow it
    net->add_subscription(0x1234);

    // Send data from a peer that never connected
    auto data_pkt = build_data_packet(1, 0, 1, 0x1234, 0, {0xAB, 0xCD});
    net->process_data_packet(peer, data_pkt.data(), data_pkt.size());

    REQUIRE_FALSE(received);
}

SCENARIO("process_data_packet rejects data for unsubscribed hashes", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    // Subscribe to hash 0x1234 but NOT 0x5678
    net->add_subscription(0x1234);

    // Establish peer
    establish_peer(*net, peer, "Peer1");

    bool received = false;
    net->set_packet_callback([&](const sock_t&, const std::string&, uint64_t, bool, std::vector<uint8_t>) {
        received = true;
    });

    // Send data with unsubscribed hash
    auto data_pkt = build_data_packet(1, 0, 1, 0x5678, 0, {0xAB, 0xCD});
    net->process_data_packet(peer, data_pkt.data(), data_pkt.size());

    REQUIRE_FALSE(received);
}

SCENARIO("process_data_packet delivers a single-fragment unreliable message", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    constexpr uint64_t HASH = 0xAAAABBBBCCCCDDDD;
    net->add_subscription(HASH);

    establish_peer(*net, peer, "Sender");

    std::vector<uint8_t> delivered;
    uint64_t delivered_hash = 0;
    bool delivered_reliable = true;

    net->set_packet_callback(
        [&](const sock_t&, const std::string&, uint64_t h, bool rel, std::vector<uint8_t>&& payload) {
            delivered      = std::move(payload);
            delivered_hash = h;
            delivered_reliable = rel;
        });

    std::vector<uint8_t> payload = {1, 2, 3, 4, 5};
    auto data_pkt = build_data_packet(42, 0, 1, HASH, 0, payload);
    net->process_data_packet(peer, data_pkt.data(), data_pkt.size());

    REQUIRE(delivered == payload);
    REQUIRE(delivered_hash == HASH);
    REQUIRE_FALSE(delivered_reliable);
}

SCENARIO("process_data_packet detects and rejects duplicate packets", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    constexpr uint64_t HASH = 0x1111222233334444;
    net->add_subscription(HASH);
    establish_peer(*net, peer, "Sender");

    int delivery_count = 0;
    net->set_packet_callback([&](const sock_t&, const std::string&, uint64_t, bool, std::vector<uint8_t>) {
        ++delivery_count;
    });

    const std::vector<uint8_t> payload = {0xDE, 0xAD};
    auto data_pkt = build_data_packet(100, 0, 1, HASH, 0, payload);

    // First delivery
    net->process_data_packet(peer, data_pkt.data(), data_pkt.size());
    REQUIRE(delivery_count == 1);

    // Duplicate — should not deliver again
    net->process_data_packet(peer, data_pkt.data(), data_pkt.size());
    REQUIRE(delivery_count == 1);
}

SCENARIO("process_data_packet rejects packets that are too short", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    net->add_subscription(0x1234);
    establish_peer(*net, peer, "Sender");

    bool received = false;
    net->set_packet_callback([&](const sock_t&, const std::string&, uint64_t, bool, std::vector<uint8_t>) {
        received = true;
    });

    // Too short to be a DataPacket
    std::array<uint8_t, 10> short_data{};
    auto* hdr        = reinterpret_cast<PacketHeader*>(short_data.data());  // NOLINT
    hdr->header[0]   = 0xE2;
    hdr->header[1]   = 0x98;
    hdr->header[2]   = 0xA2;
    hdr->version     = PROTOCOL_VERSION;
    hdr->type        = DATA;

    net->process_data_packet(peer, short_data.data(), short_data.size());
    REQUIRE_FALSE(received);
}

SCENARIO("process_data_packet reassembles multi-fragment messages", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    constexpr uint64_t HASH = 0xFEDCBA9876543210;
    net->add_subscription(HASH);
    establish_peer(*net, peer, "Sender");

    std::vector<uint8_t> delivered;
    net->set_packet_callback(
        [&](const sock_t&, const std::string&, uint64_t, bool, std::vector<uint8_t>&& payload) {
            delivered = std::move(payload);
        });

    // Send 3 fragments of a 15-byte message (5 bytes each)
    const std::vector<uint8_t> frag0 = {0, 1, 2, 3, 4};
    const std::vector<uint8_t> frag1 = {5, 6, 7, 8, 9};
    const std::vector<uint8_t> frag2 = {10, 11, 12, 13, 14};

    auto pkt0 = build_data_packet(200, 0, 3, HASH, 0, frag0);
    auto pkt1 = build_data_packet(200, 1, 3, HASH, 0, frag1);
    auto pkt2 = build_data_packet(200, 2, 3, HASH, 0, frag2);

    net->process_data_packet(peer, pkt0.data(), pkt0.size());
    REQUIRE(delivered.empty());

    net->process_data_packet(peer, pkt1.data(), pkt1.size());
    REQUIRE(delivered.empty());

    net->process_data_packet(peer, pkt2.data(), pkt2.size());

    std::vector<uint8_t> expected = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    REQUIRE(delivered == expected);
}


// ===========================================================================
// process_ack_packet behavioral tests
// ===========================================================================

SCENARIO("process_ack_packet rejects ACKs from unconnected peers", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);

    // Build an ACK but don't connect the peer
    const std::vector<bool> received_bits(1, true);
    auto ack_pkt = build_ack_packet(1, 1, received_bits);

    // Should not crash — just silently discard
    net->process_ack_packet(peer, ack_pkt.data(), ack_pkt.size());
}

SCENARIO("process_ack_packet rejects packets that are too short", "[nuclearnet][process_packet]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);
    establish_peer(*net, peer, "Peer1");

    // Too short for ACKPacket
    std::array<uint8_t, 5> short_data{};
    auto* hdr        = reinterpret_cast<PacketHeader*>(short_data.data());  // NOLINT
    hdr->header[0]   = 0xE2;
    hdr->header[1]   = 0x98;
    hdr->header[2]   = 0xA2;
    hdr->version     = PROTOCOL_VERSION;
    hdr->type        = ACK;

    // Should not crash
    net->process_ack_packet(peer, short_data.data(), short_data.size());
}


// ===========================================================================
// send() behavioral tests
// ===========================================================================

SCENARIO("send does nothing when sockets are not initialized", "[nuclearnet][send]") {
    // A NUClearNet that has never had reset() called has no valid fd
    auto net = std::make_unique<NUClearNet>();

    // Should not crash or throw
    std::vector<uint8_t> payload = {1, 2, 3};
    net->send(0x1234, payload.data(), payload.size(), "", false);
}

SCENARIO("send to named peer that does not exist delivers nothing", "[nuclearnet][send]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("No multicast support");
    }

    auto net    = make_test_net();
    const sock_t peer = make_addr(0x0A000001, 5000);
    establish_peer(*net, peer, "ExistingPeer");

    // Try to send to a non-existent peer name — should not crash
    std::vector<uint8_t> payload = {1, 2, 3};
    net->send(0x1234, payload.data(), payload.size(), "NonExistentPeer", false);
}
