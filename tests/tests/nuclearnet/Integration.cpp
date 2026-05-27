/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
 * THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nuclearnet/NUClearNet.hpp"

#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "test_util/has_multicast.hpp"

using namespace std::chrono_literals;

namespace {

using NUClear::network::NUClearNet;
using NUClear::network::NetworkConfig;
using NUClear::network::PeerInfo;
using NUClear::util::network::sock_t;

constexpr uint64_t HASH_A = 0x1111'2222'3333'4444ULL;
constexpr uint64_t HASH_B = 0x5555'6666'7777'8888ULL;

NetworkConfig make_config(const std::string& name) {
    NetworkConfig config;
    config.name             = name;
    config.announce_address = "239.226.152.162";
    config.announce_port    = 17747;
    config.mtu              = 1500;
    config.peer_timeout     = 1s;
    return config;
}

bool wait_for(const std::function<bool()>& predicate,
              std::chrono::milliseconds timeout,
              const std::function<void()>& pump) {
    auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) {
            return true;
        }
        pump();
        std::this_thread::sleep_for(5ms);
    }
    return predicate();
}

struct NetworkPair {
    NUClearNet a;
    NUClearNet b;

    ~NetworkPair() {
        a.shutdown();
        b.shutdown();
    }
};

std::vector<uint8_t> make_payload(std::size_t size, uint8_t seed) {
    std::vector<uint8_t> payload(size);
    for (std::size_t i = 0; i < payload.size(); ++i) {
        payload[i] = static_cast<uint8_t>(seed + static_cast<uint8_t>(i));
    }
    return payload;
}

}  // namespace

SCENARIO("Two NUClearNet instances discover and exchange messages", "[nuclearnet][integration]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("IPv4 multicast is unavailable on this system");
    }

    NetworkPair net;

    net.a.reset(make_config("alpha"));
    net.b.reset(make_config("bravo"));

    // Each peer subscribes to the other's hash so routing has something real to filter on.
    net.a.set_subscriptions({HASH_B});
    net.b.set_subscriptions({HASH_A});

    std::mutex mutex;
    std::vector<std::string> join_events;
    std::vector<std::string> leave_events;
    std::vector<std::pair<std::string, std::vector<uint8_t>>> received;

    net.a.set_join_callback([&](const PeerInfo& peer) {
        std::lock_guard<std::mutex> lock(mutex);
        join_events.push_back("a:" + peer.name);
    });
    net.b.set_join_callback([&](const PeerInfo& peer) {
        std::lock_guard<std::mutex> lock(mutex);
        join_events.push_back("b:" + peer.name);
    });

    net.a.set_leave_callback([&](const PeerInfo& peer) {
        std::lock_guard<std::mutex> lock(mutex);
        leave_events.push_back("a:" + peer.name);
    });
    net.b.set_leave_callback([&](const PeerInfo& peer) {
        std::lock_guard<std::mutex> lock(mutex);
        leave_events.push_back("b:" + peer.name);
    });

    net.a.set_packet_callback([&](const sock_t&, const std::string& peer_name, uint64_t hash, bool reliable,
                                  std::vector<uint8_t>&& payload) {
        std::lock_guard<std::mutex> lock(mutex);
        received.emplace_back("a:" + peer_name + ":" + std::to_string(hash) + ":" + (reliable ? "1" : "0"),
                              std::move(payload));
    });
    net.b.set_packet_callback([&](const sock_t&, const std::string& peer_name, uint64_t hash, bool reliable,
                                  std::vector<uint8_t>&& payload) {
        std::lock_guard<std::mutex> lock(mutex);
        received.emplace_back("b:" + peer_name + ":" + std::to_string(hash) + ":" + (reliable ? "1" : "0"),
                              std::move(payload));
    });

    REQUIRE(wait_for([&] {
        std::lock_guard<std::mutex> lock(mutex);
        return std::find(join_events.begin(), join_events.end(), "a:bravo") != join_events.end()
               && std::find(join_events.begin(), join_events.end(), "b:alpha") != join_events.end();
    }, 5s, [&] {
        net.a.process();
        net.b.process();
    }));

    auto payload_a = make_payload(4096, 0x10);
    auto payload_b = make_payload(64, 0x80);

    net.a.send(HASH_A, payload_a.data(), payload_a.size(), "", true);
    net.b.send(HASH_B, payload_b.data(), payload_b.size(), "", false);

    REQUIRE(wait_for([&] {
        std::lock_guard<std::mutex> lock(mutex);
        return received.size() == 2;
    }, 5s, [&] {
        net.a.process();
        net.b.process();
    }));

    {
        std::lock_guard<std::mutex> lock(mutex);
        REQUIRE(received.size() == 2);

        const auto expected_from_b = std::string("b:alpha:") + std::to_string(HASH_A) + ":1";
        const auto expected_from_a = std::string("a:bravo:") + std::to_string(HASH_B) + ":0";

        auto it_a = std::find_if(received.begin(), received.end(), [&expected_from_b](const auto& entry) {
            return entry.first == expected_from_b;
        });
        auto it_b = std::find_if(received.begin(), received.end(), [&expected_from_a](const auto& entry) {
            return entry.first == expected_from_a;
        });

        REQUIRE(it_a != received.end());
        REQUIRE(it_b != received.end());
        REQUIRE(it_a->second == payload_a);
        REQUIRE(it_b->second == payload_b);
    }

    net.b.shutdown();

    REQUIRE(wait_for([&] {
        std::lock_guard<std::mutex> lock(mutex);
        return std::find(leave_events.begin(), leave_events.end(), "a:bravo") != leave_events.end();
    }, 5s, [&] {
        net.a.process();
    }));
}

SCENARIO("NUClearNet handles bidirectional reliable traffic", "[nuclearnet][integration]") {
    if (!test_util::has_ipv4_multicast()) {
        SKIP("IPv4 multicast is unavailable on this system");
    }

    NetworkPair net;

    net.a.reset(make_config("left"));
    net.b.reset(make_config("right"));

    net.a.set_subscriptions({HASH_B});
    net.b.set_subscriptions({HASH_A});

    std::mutex mutex;
    std::vector<std::string> join_events;
    std::vector<std::vector<uint8_t>> a_received;
    std::vector<std::vector<uint8_t>> b_received;

    net.a.set_join_callback([&](const PeerInfo& peer) {
        std::lock_guard<std::mutex> lock(mutex);
        join_events.push_back("a:" + peer.name);
    });
    net.b.set_join_callback([&](const PeerInfo& peer) {
        std::lock_guard<std::mutex> lock(mutex);
        join_events.push_back("b:" + peer.name);
    });

    net.a.set_packet_callback([&](const sock_t&, const std::string&, uint64_t, bool, std::vector<uint8_t>&& payload) {
        std::lock_guard<std::mutex> lock(mutex);
        a_received.push_back(std::move(payload));
    });
    net.b.set_packet_callback([&](const sock_t&, const std::string&, uint64_t, bool, std::vector<uint8_t>&& payload) {
        std::lock_guard<std::mutex> lock(mutex);
        b_received.push_back(std::move(payload));
    });

    REQUIRE(wait_for([&] {
        std::lock_guard<std::mutex> lock(mutex);
        return std::find(join_events.begin(), join_events.end(), "a:right") != join_events.end()
               && std::find(join_events.begin(), join_events.end(), "b:left") != join_events.end();
    }, 5s, [&] {
        net.a.process();
        net.b.process();
    }));

    auto large_payload = make_payload(8192, 0x33);
    auto small_payload = make_payload(32, 0x44);

    net.a.send(HASH_A, large_payload.data(), large_payload.size(), "", true);
    net.b.send(HASH_B, small_payload.data(), small_payload.size(), "", true);

    REQUIRE(wait_for([&] {
        std::lock_guard<std::mutex> lock(mutex);
        return a_received.size() == 1 && b_received.size() == 1;
    }, 5s, [&] {
        net.a.process();
        net.b.process();
    }));

    {
        std::lock_guard<std::mutex> lock(mutex);
        REQUIRE(a_received[0] == small_payload);
        REQUIRE(b_received[0] == large_payload);
    }
}
