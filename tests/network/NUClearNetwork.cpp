/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#include <catch.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

/// @brief Events that occur during the test
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

struct IPv4UnicastConfig {
    static std::string hostname() {}
};

struct IPv6UnicastConfig {
    static std::string hostname() {}
};

struct IPv4BroadcastConfig {
    static std::string hostname() {}
};

struct IPv4MulticastConfig {
    static std::string hostname() {}
};

struct IPv6MulticastConfig {
    static std::string hostname() {}
};

template <typename TestConfig, char hostname>
class NetworkBase : public test_util::TestBase<TestReactor> {

    // Set our function callback
    network.set_packet_callback([this](const network::NUClearNetwork::NetworkTarget& remote,
                                       const uint64_t& hash,
                                       const bool& reliable,
                                       std::vector<uint8_t>&& payload) {
        emit(std::make_unique<Packet>(remote, hash, reliable, std::move(payload)));
    });

    // Set our join callback
    network.set_join_callback([this](const network::NUClearNetwork::NetworkTarget& remote) {
        // Join event
    });

    // Set our leave callback
    network.set_leave_callback([this](const network::NUClearNetwork::NetworkTarget& remote) {
        // Leave event
    });

    // Set our event timer callback
    network.set_next_event_callback([this](std::chrono::steady_clock::time_point t) {
        const std::chrono::steady_clock::duration emit_offset = t - std::chrono::steady_clock::now();
        emit<Scope::DELAY>(std::make_unique<ProcessNetwork>(),
                           std::chrono::duration_cast<NUClear::clock::duration>(emit_offset));
    });

public:
    explicit TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Reset both networks to connect them
        network_a.reset(name, "127.0.0.1", 7447);
        network_b.reset(name, "127.0.0.1", 7447);

        // Execution handle
        process_handle = on<Trigger<ProcessNetwork>>().then("Network processing", [this] { network.process(); });

        for (auto& fd : network.listen_fds()) {
            listen_handles.push_back(on<IO>(fd, IO::READ).then("Packet", [this] { network.process(); }));
        }
    }

    NUClear::extension::network::NUClearNetwork network;
};
}  // namespace


TEST_CASE("Testing that when an on statement does not have it's data satisfied it does not run", "[api][nodata]") {

    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Emitting MessageA",
        "MessageA triggered",
        "Emitting MessageB",
        "MessageB with MessageA triggered",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
