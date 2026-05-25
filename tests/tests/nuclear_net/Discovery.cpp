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

#include "nuclear_net/Discovery.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>
#include <vector>

#include "util/platform.hpp"

using NUClear::network::Discovery;
using NUClear::network::PeerInfo;
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

TEST_CASE("Discovery build_announce_packet produces valid packet", "[nuclear_net][discovery]") {
    auto packet = Discovery::build_announce_packet("test_node", {0x1111, 0x2222});

    // Should start with the magic bytes (☢ = 0xE2, 0x98, 0xA2)
    REQUIRE(packet.size() >= 5);
    REQUIRE(packet[0] == 0xE2);
    REQUIRE(packet[1] == 0x98);
    REQUIRE(packet[2] == 0xA2);
}

TEST_CASE("Discovery build_leave_packet produces valid packet", "[nuclear_net][discovery]") {
    auto packet = Discovery::build_leave_packet();

    // Should have the magic bytes and LEAVE type
    REQUIRE(packet.size() >= 5);
    REQUIRE(packet[0] == 0xE2);
    REQUIRE(packet[1] == 0x98);
    REQUIRE(packet[2] == 0xA2);
}

TEST_CASE("Discovery process_announce adds a new peer", "[nuclear_net][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool join_called = false;
    std::string joined_name;

    disc.set_join_callback([&](const PeerInfo& info) {
        join_called = true;
        joined_name = info.name;
    });

    auto announce = Discovery::build_announce_packet("peer_a", {0x1111});
    disc.process_announce(peer_addr, announce.data(), announce.size());

    REQUIRE(join_called);
    REQUIRE(joined_name == "peer_a");
    REQUIRE(disc.has_peer(peer_addr));

    const auto* peer = disc.get_peer(peer_addr);
    REQUIRE(peer != nullptr);
    REQUIRE(peer->name == "peer_a");
    REQUIRE(peer->subscriptions.count(0x1111) == 1);
}

TEST_CASE("Discovery process_announce updates existing peer subscriptions", "[nuclear_net][discovery]") {
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
    const auto* peer = disc.get_peer(peer_addr);
    REQUIRE(peer != nullptr);
    REQUIRE(peer->subscriptions.count(0x2222) == 1);
    REQUIRE(peer->subscriptions.count(0x3333) == 1);
    REQUIRE(peer->subscriptions.count(0x1111) == 0);
}

TEST_CASE("Discovery process_leave removes a peer", "[nuclear_net][discovery]") {
    Discovery disc(std::chrono::seconds(5));

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool leave_called = false;

    disc.set_leave_callback([&](const PeerInfo& info) {
        leave_called = true;
    });

    // First, add the peer
    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());
    REQUIRE(disc.has_peer(peer_addr));

    // Now process a leave
    disc.process_leave(peer_addr);

    REQUIRE(leave_called);
    REQUIRE_FALSE(disc.has_peer(peer_addr));
}

TEST_CASE("Discovery check_timeouts removes stale peers", "[nuclear_net][discovery]") {
    Discovery disc(std::chrono::milliseconds(20));  // 20ms timeout for testing

    sock_t peer_addr = make_addr(0x0A000001, 5000);
    bool leave_called = false;

    disc.set_leave_callback([&](const PeerInfo&) { leave_called = true; });

    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    auto removed = disc.check_timeouts();
    REQUIRE(removed.size() == 1);
    REQUIRE(removed[0].name == "peer_a");
    REQUIRE(leave_called);
    REQUIRE_FALSE(disc.has_peer(peer_addr));
}

TEST_CASE("Discovery touch_peer resets timeout", "[nuclear_net][discovery]") {
    Discovery disc(std::chrono::milliseconds(50));

    sock_t peer_addr = make_addr(0x0A000001, 5000);

    auto announce = Discovery::build_announce_packet("peer_a", {});
    disc.process_announce(peer_addr, announce.data(), announce.size());

    // Wait part of the timeout, then touch
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    disc.touch_peer(peer_addr);

    // Wait another partial timeout (total would have expired without touch)
    std::this_thread::sleep_for(std::chrono::milliseconds(30));

    auto removed = disc.check_timeouts();
    REQUIRE(removed.empty());
    REQUIRE(disc.has_peer(peer_addr));
}

TEST_CASE("Discovery get_peers returns all known peers", "[nuclear_net][discovery]") {
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
