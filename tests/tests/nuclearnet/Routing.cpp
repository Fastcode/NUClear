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

#include "nuclearnet/Routing.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <set>
#include <vector>

#include "util/platform.hpp"

using NUClear::network::Routing;
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

SCENARIO("Routing delivers to peers subscribed to the message hash", "[nuclearnet][routing]") {
    Routing routing;

    sock_t peer_a = make_addr(0x0A000001, 5000);
    sock_t peer_b = make_addr(0x0A000002, 5000);

    routing.update_peer_subscriptions(peer_a, {0x1111, 0x2222});
    routing.update_peer_subscriptions(peer_b, {0x2222, 0x3333});

    // Hash 0x1111 — only peer_a
    REQUIRE(routing.should_send(peer_a, 0x1111));
    REQUIRE_FALSE(routing.should_send(peer_b, 0x1111));

    // Hash 0x2222 — both peers
    REQUIRE(routing.should_send(peer_a, 0x2222));
    REQUIRE(routing.should_send(peer_b, 0x2222));

    // Hash 0x3333 — only peer_b
    REQUIRE_FALSE(routing.should_send(peer_a, 0x3333));
    REQUIRE(routing.should_send(peer_b, 0x3333));
}

SCENARIO("Routing delivers all messages when peer has empty subscription set", "[nuclearnet][routing]") {
    Routing routing;

    sock_t peer = make_addr(0x0A000001, 5000);

    // Empty subscription set = receive everything
    routing.update_peer_subscriptions(peer, {});

    REQUIRE(routing.should_send(peer, 0x1111));
    REQUIRE(routing.should_send(peer, 0x9999));
    REQUIRE(routing.should_send(peer, 0));
}

SCENARIO("Routing allows sending to unknown peers by default", "[nuclearnet][routing]") {
    Routing routing;

    sock_t unknown = make_addr(0x0A000099, 5000);

    // Unknown peer — should default to allowing sends
    REQUIRE(routing.should_send(unknown, 0x1111));
}

SCENARIO("Routing get_targets returns all subscribed peers for a hash", "[nuclearnet][routing]") {
    Routing routing;

    sock_t peer_a = make_addr(0x0A000001, 5000);
    sock_t peer_b = make_addr(0x0A000002, 5000);
    sock_t peer_c = make_addr(0x0A000003, 5000);

    routing.update_peer_subscriptions(peer_a, {0x1111});
    routing.update_peer_subscriptions(peer_b, {0x1111, 0x2222});
    routing.update_peer_subscriptions(peer_c, {0x2222});

    std::vector<sock_t> all_peers = {peer_a, peer_b, peer_c};
    auto targets = routing.get_targets(all_peers, 0x1111);
    REQUIRE(targets.size() == 2);
}

SCENARIO("Routing local subscriptions are tracked correctly", "[nuclearnet][routing]") {
    Routing routing;

    routing.set_local_subscriptions({0x1111, 0x2222, 0x3333});
    auto subs = routing.get_local_subscriptions();
    REQUIRE(subs.size() == 3);
    REQUIRE(std::find(subs.begin(), subs.end(), 0x1111) != subs.end());
    REQUIRE(std::find(subs.begin(), subs.end(), 0x2222) != subs.end());
    REQUIRE(std::find(subs.begin(), subs.end(), 0x3333) != subs.end());

    routing.add_local_subscription(0x4444);
    subs = routing.get_local_subscriptions();
    REQUIRE(subs.size() == 4);
    REQUIRE(std::find(subs.begin(), subs.end(), 0x4444) != subs.end());
}

SCENARIO("Routing removes peer correctly", "[nuclearnet][routing]") {
    Routing routing;

    sock_t peer = make_addr(0x0A000001, 5000);

    routing.update_peer_subscriptions(peer, {0x1111});
    REQUIRE(routing.should_send(peer, 0x1111));
    REQUIRE_FALSE(routing.should_send(peer, 0x9999));  // Not subscribed

    routing.remove_peer(peer);

    // After removal, peer is unknown again — defaults to allowing sends
    REQUIRE(routing.should_send(peer, 0x1111));
    REQUIRE(routing.should_send(peer, 0x9999));
}

SCENARIO("Routing updates subscriptions for existing peer", "[nuclearnet][routing]") {
    Routing routing;

    sock_t peer = make_addr(0x0A000001, 5000);

    routing.update_peer_subscriptions(peer, {0x1111});
    REQUIRE(routing.should_send(peer, 0x1111));
    REQUIRE_FALSE(routing.should_send(peer, 0x2222));

    // Update subscriptions
    routing.update_peer_subscriptions(peer, {0x2222});
    REQUIRE_FALSE(routing.should_send(peer, 0x1111));
    REQUIRE(routing.should_send(peer, 0x2222));
}
