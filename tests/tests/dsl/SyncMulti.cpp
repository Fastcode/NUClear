/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nuclear"
#include "test_util/TestBase.hpp"
#include "test_util/TimeUnit.hpp"
#include "test_util/common.hpp"
#include "util/precise_sleep.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct A {};
    struct B {};

    void do_task(const std::string& event) {
        events.emplace_back(event + " started");
        // Sleep for a bit to give a chance for the other threads to cause problems
        NUClear::util::precise_sleep(test_util::TimeUnit(2));
        events.emplace_back(event + " finished");
    }

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<A>, Sync<A>>().then([this] { do_task("Sync A"); });
        on<Trigger<A>, Sync<A>, Sync<B>>().then([this] { do_task("Sync Both"); });
        on<Trigger<B>, Sync<B>>().then([this] { do_task("Sync B"); });

        on<Startup>().then([this] {
            start_time = std::chrono::steady_clock::now();
            // Emitting both A and B at the same time should trigger all the reactions, but they should execute in order
            emit(std::make_unique<A>());
            emit(std::make_unique<B>());
        });
    }

    std::chrono::steady_clock::time_point start_time;

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Test that sync works when one thread has multiple groups", "[api][sync][multi]") {
    NUClear::Configuration config;
    config.default_pool_concurrency = 4;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Sync A started",
        "Sync A finished",
        "Sync Both started",
        "Sync Both finished",
        "Sync B started",
        "Sync B finished",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
