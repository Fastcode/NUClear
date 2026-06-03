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

#include "nuclearnet/Discovery.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <vector>

#include "nuclearnet/wire_protocol.hpp"
#include "util/platform.hpp"

using NUClear::network::HandshakeState;
using NUClear::network::Discovery;
using NUClear::network::PeerInfo;
using NUClear::network::SYN;
using NUClear::network::CON_ACK;
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

SCENARIO("Discovery build_announce_packet produces valid packet", "[nuclearnet][discovery]") {
    auto packet = Discovery::build_announce_packet("test_node", {0x1111, 0x2222});

    // Should start with the magic bytes (☢ = 0xE2, 0x98, 0xA2)
    REQUIRE(packet.size() >= 5);
    REQUIRE(packet[0] == 0xE2);
    REQUIRE(packet[1] == 0x98);
    REQUIRE(packet[2] == 0xA2);
}

SCENARIO("Discovery build_leave_packet produces valid packet", "[nuclearnet][discovery]") {
    auto packet = Discovery::build_leave_packet();

    // Should have the magic bytes and LEAVE type
    REQUIRE(packet.size() >= 5);
    REQUIRE(packet[0] == 0xE2);
    REQUIRE(packet[1] == 0x98);
    REQUIRE(packet[2] == 0xA2);
}

SCENARIO("Discovery process_announce adds a new peer", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool join_called = false;

    disc.set_join_callback([&](const PeerInfo& info) {
        join_called = true;
    });

    auto announce = Discovery::build_announce_packet("peer_a", {0x1111});
    disc.process_announce(peer_addr, announce.data(), announce.size());

    // Join is deferred until handshake completes
    REQUIRE_FALSE(join_called);
    REQUIRE(disc.has_peer(peer_addr));

    PeerInfo peer;
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.name == "peer_a");
    REQUIRE(peer.subscriptions.count(0x1111) == 1);
    REQUIRE(peer.announce_heard);
    REQUIRE(peer.handshake == HandshakeState::IDLE);
}

SCENARIO("Discovery process_announce updates existing peer subscriptions", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool sub_changed = false;

    disc.set_subscription_change_callback([&](const PeerInfo&) { sub_changed = true; });

    auto announce1 = Discovery::build_announce_packet("peer_a", {0x1111});
    disc.process_announce(peer_addr, announce1.data(), announce1.size());

    // Send a new announce with different subscriptions
    sub_changed    = false;
    auto announce2 = Discovery::build_announce_packet("peer_a", {0x2222, 0x3333});
    disc.process_announce(peer_addr, announce2.data(), announce2.size());

    REQUIRE(sub_changed);
    PeerInfo peer;
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.subscriptions.count(0x2222) == 1);
    REQUIRE(peer.subscriptions.count(0x3333) == 1);
    REQUIRE(peer.subscriptions.count(0x1111) == 0);
}

SCENARIO("Discovery process_leave removes a peer", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool leave_called = false;

    disc.set_leave_callback([&](const PeerInfo& info) {
        leave_called = true;
    });

    // Add the peer and complete the handshake
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());
    disc.mark_syn_sent(peer_addr);
    disc.process_connect(peer_addr, SYN | CON_ACK);  // SYN+ACK response
    REQUIRE(disc.is_connected(peer_addr));

    // Now process a leave
    disc.process_leave(peer_addr);

    REQUIRE(leave_called);
    REQUIRE_FALSE(disc.has_peer(peer_addr));
}

SCENARIO("Discovery check_timeouts removes stale peers", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::milliseconds(20));  // 20ms timeout for testing

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool leave_called = false;

    disc.set_leave_callback([&](const PeerInfo&) { leave_called = true; });

    // Add peer at time T and complete handshake
    auto t = std::chrono::steady_clock::now();
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size(), t);
    disc.mark_syn_sent(peer_addr);
    disc.process_connect(peer_addr, SYN | CON_ACK, t);

    // Check at T+10ms (before timeout) — peer should still be there
    auto removed = disc.check_timeouts(t + std::chrono::milliseconds(10));
    REQUIRE(removed.empty());
    REQUIRE(disc.has_peer(peer_addr));

    // Check at T+25ms (after 20ms timeout) — peer should be removed
    removed = disc.check_timeouts(t + std::chrono::milliseconds(25));
    REQUIRE(removed.size() == 1);
    REQUIRE(removed[0].name == "peer_a");
    REQUIRE(leave_called);
    REQUIRE_FALSE(disc.has_peer(peer_addr));
}

SCENARIO("Discovery touch_peer resets timeout", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::milliseconds(200));

    sock_t peer_addr = make_addr(0x0A000001, 5000);

    // Add peer at time T and complete handshake
    auto t = std::chrono::steady_clock::now();
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size(), t);
    disc.mark_syn_sent(peer_addr);
    disc.process_connect(peer_addr, SYN | CON_ACK, t);

    // Touch at T+120ms (before 200ms timeout expires)
    disc.touch_peer(peer_addr, t + std::chrono::milliseconds(120));

    // Check at T+240ms — 240ms since announce, but only 120ms since touch
    // Since timeout is 200ms from last_seen, peer should still be alive
    auto removed = disc.check_timeouts(t + std::chrono::milliseconds(240));
    REQUIRE(removed.empty());
    REQUIRE(disc.has_peer(peer_addr));

    // Check at T+325ms — 205ms since touch, should now be timed out
    removed = disc.check_timeouts(t + std::chrono::milliseconds(325));
    REQUIRE(removed.size() == 1);
    REQUIRE(removed[0].name == "peer_a");
}

SCENARIO("Discovery get_peers returns all known peers", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t addr_a = make_addr(0x0A000001, 5000);
    sock_t addr_b = make_addr(0x0A000002, 5000);

    auto announce_a = Discovery::build_announce_packet("node_a", {0x1111});
    auto announce_b = Discovery::build_announce_packet("node_b", {0x2222});

    disc.process_announce(addr_a, announce_a.data(), announce_a.size());
    disc.process_announce(addr_b, announce_b.data(), announce_b.size());

    auto peers = disc.get_peers();
    REQUIRE(peers.size() == 2);
    REQUIRE(peers.count(addr_a) == 1);
    REQUIRE(peers.count(addr_b) == 1);
    REQUIRE(peers[addr_a].name == "node_a");
    REQUIRE(peers[addr_b].name == "node_b");
}

SCENARIO("Discovery 3-way handshake normal flow", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool join_called = false;
    std::string joined_name;

    disc.set_join_callback([&](const PeerInfo& info) {
        join_called = true;
        joined_name = info.name;
    });

    // Peer announces (heard on announce channel — sets announce_heard)
    auto announce = Discovery::build_announce_packet("peer_a", {0x1111});
    disc.process_announce(peer_addr, announce.data(), announce.size());
    REQUIRE_FALSE(join_called);
    PeerInfo peer;
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.announce_heard);
    REQUIRE(peer.handshake == HandshakeState::IDLE);

    // We send SYN
    disc.mark_syn_sent(peer_addr);
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.handshake == HandshakeState::SYN_SENT);

    // Peer responds with SYN+ACK
    auto result = disc.process_connect(peer_addr, SYN | CON_ACK);
    REQUIRE(result.just_connected);
    REQUIRE(result.response_flags == CON_ACK);  // We should send ACK back
    REQUIRE(join_called);
    REQUIRE(joined_name == "peer_a");
    REQUIRE(disc.is_connected(peer_addr));
}

SCENARIO("Discovery 3-way handshake receiving SYN first", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool join_called = false;

    disc.set_join_callback([&](const PeerInfo&) { join_called = true; });

    // Peer announces and we add them
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());

    // Peer sends SYN to us (they initiated)
    auto result = disc.process_connect(peer_addr, SYN);
    REQUIRE_FALSE(result.just_connected);
    REQUIRE(result.response_flags == (SYN | CON_ACK));  // We respond with SYN+ACK
    REQUIRE_FALSE(join_called);
    PeerInfo peer;
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.handshake == HandshakeState::SYN_RECEIVED);

    // Peer sends ACK to complete the handshake
    result = disc.process_connect(peer_addr, CON_ACK);
    REQUIRE(result.just_connected);
    REQUIRE(result.response_flags == 0);  // No further response needed
    REQUIRE(join_called);
    REQUIRE(disc.is_connected(peer_addr));
}

SCENARIO("Discovery 3-way handshake simultaneous open", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool join_called = false;

    disc.set_join_callback([&](const PeerInfo&) { join_called = true; });

    // Peer announces
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());

    // We send SYN
    disc.mark_syn_sent(peer_addr);

    // But peer also sent SYN at the same time (simultaneous open)
    auto result = disc.process_connect(peer_addr, SYN);
    REQUIRE_FALSE(result.just_connected);
    REQUIRE(result.response_flags == (SYN | CON_ACK));  // Respond with SYN+ACK
    PeerInfo peer;
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.handshake == HandshakeState::SYN_RECEIVED);

    // Peer also sends SYN+ACK (they got our SYN)
    result = disc.process_connect(peer_addr, SYN | CON_ACK);
    REQUIRE(result.just_connected);
    REQUIRE(result.response_flags == CON_ACK);
    REQUIRE(join_called);
    REQUIRE(disc.is_connected(peer_addr));
}

SCENARIO("Discovery build_connect_packet produces valid packet", "[nuclearnet][discovery]") {
    auto syn_packet = Discovery::build_connect_packet(SYN);
    REQUIRE(syn_packet.size() == 6);
    REQUIRE(syn_packet[0] == 0xE2);
    REQUIRE(syn_packet[1] == 0x98);
    REQUIRE(syn_packet[2] == 0xA2);
    REQUIRE(syn_packet[4] == 5);  // CONNECT type
    REQUIRE(syn_packet[5] == SYN);

    auto synack_packet = Discovery::build_connect_packet(SYN | CON_ACK);
    REQUIRE(synack_packet[5] == (SYN | CON_ACK));
}

SCENARIO("Discovery process_leave does not fire callback for non-connected peer", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool leave_called = false;

    disc.set_leave_callback([&](const PeerInfo&) { leave_called = true; });

    // Add peer but do NOT complete handshake
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());

    disc.process_leave(peer_addr);

    REQUIRE_FALSE(leave_called);
    REQUIRE_FALSE(disc.has_peer(peer_addr));
}

SCENARIO("Discovery connection deferred until announce heard", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool join_called = false;

    disc.set_join_callback([&](const PeerInfo&) { join_called = true; });

    // Peer sends SYN before we've heard their announce (they added us via CONNECT)
    auto result = disc.process_connect(peer_addr, SYN);
    REQUIRE(result.response_flags == (SYN | CON_ACK));
    REQUIRE_FALSE(join_called);

    // Complete the data handshake
    result = disc.process_connect(peer_addr, CON_ACK);
    // Data path is confirmed but announce not yet heard — NOT connected
    REQUIRE_FALSE(result.just_connected);
    REQUIRE_FALSE(join_called);
    REQUIRE_FALSE(disc.is_connected(peer_addr));
    PeerInfo peer;
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.handshake == HandshakeState::CONFIRMED);
    REQUIRE_FALSE(peer.announce_heard);

    // Now we hear their announce on the announce channel
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());

    // NOW the connection should be up
    REQUIRE(join_called);
    REQUIRE(disc.is_connected(peer_addr));
}

SCENARIO("Discovery retransmits SYN when announce received in SYN_SENT state", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);

    // First announce — new peer
    auto announce = Discovery::build_announce_packet("peer_a", {});
    auto result = disc.process_announce(peer_addr, announce.data(), announce.size());
    REQUIRE(result.is_new);

    // We send SYN (externally) and mark state
    disc.mark_syn_sent(peer_addr);
    PeerInfo peer;
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.handshake == HandshakeState::SYN_SENT);

    // SYN was dropped. Another announce arrives — should indicate SYN retransmit
    result = disc.process_announce(peer_addr, announce.data(), announce.size());
    REQUIRE_FALSE(result.is_new);
    REQUIRE(result.response_flags == SYN);
}

SCENARIO("Discovery retransmits SYN+ACK when announce received in SYN_RECEIVED state", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);

    // Add peer via announce
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());

    // Peer sends SYN — we go to SYN_RECEIVED
    auto connect_result = disc.process_connect(peer_addr, SYN);
    REQUIRE(connect_result.response_flags == (SYN | CON_ACK));
    PeerInfo peer;
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.handshake == HandshakeState::SYN_RECEIVED);

    // Our SYN+ACK was dropped. Another announce arrives — should indicate SYN+ACK retransmit
    auto result = disc.process_announce(peer_addr, announce.data(), announce.size());
    REQUIRE_FALSE(result.is_new);
    REQUIRE(result.response_flags == (SYN | CON_ACK));
}

SCENARIO("Discovery retransmits ACK when announce received in CONFIRMED but peer not connected", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);

    // Add peer via announce and complete handshake
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());
    disc.mark_syn_sent(peer_addr);
    auto connect_result = disc.process_connect(peer_addr, SYN | CON_ACK);
    REQUIRE(connect_result.just_connected);
    REQUIRE(disc.is_connected(peer_addr));

    // Peer is fully connected — no retransmit needed
    auto result = disc.process_announce(peer_addr, announce.data(), announce.size());
    REQUIRE_FALSE(result.is_new);
    REQUIRE(result.response_flags == CON_ACK);
}

SCENARIO("Discovery no retransmit for IDLE peer (not yet sent SYN)", "[nuclearnet][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);

    // First announce — new peer, handshake IDLE
    auto announce = Discovery::build_announce_packet("peer_a", {});
    auto result = disc.process_announce(peer_addr, announce.data(), announce.size());
    REQUIRE(result.is_new);
    PeerInfo peer;
    REQUIRE(disc.get_peer(peer_addr, peer));
    REQUIRE(peer.handshake == HandshakeState::IDLE);

    // Second announce — peer still in IDLE (we haven't sent SYN yet)
    // Should indicate SYN needed
    result = disc.process_announce(peer_addr, announce.data(), announce.size());
    REQUIRE_FALSE(result.is_new);
    REQUIRE(result.response_flags == SYN);
}
