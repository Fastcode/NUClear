/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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
#include <memory>
#include <nuclear>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "test_util/TestBase.hpp"
#include "test_util/TimeUnit.hpp"
#include "test_util/common.hpp"
#include "util/precise_sleep.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    explicit TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Trigger<Step<1>>, MainThread>().then([this] {  //
            main_thread_id = std::this_thread::get_id();
            events.push_back("Step<1>");
        });
        on<Idle<MainThread>>().then([this] {
            idle_thread_id = std::this_thread::get_id();
            events.push_back("Main Idle");
            emit(std::make_unique<Step<2>>());
        });
        on<Trigger<Step<2>>>().then([this] {
            default_thread_id = std::this_thread::get_id();
            events.push_back("Step<2>");
            // Sleep for a bit to coax out any more idle triggers
            NUClear::util::precise_sleep(test_util::TimeUnit(2));
            powerplant.shutdown();
        });

        on<Startup>().then([this] { emit(std::make_unique<Step<1>>()); });
    }

    std::thread::id main_thread_id;
    std::thread::id idle_thread_id;
    std::thread::id default_thread_id;

    /// A vector of events that have happened
    std::vector<std::string> events;
};


TEST_CASE("Test that idle can fire events for other pools but only runs once", "[api][dsl][Idle][Pool]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    auto& reactor = plant.install<TestReactor>();
    plant.start();

    // Check that things ran on the correct thread
    CAPTURE(std::this_thread::get_id());
    CHECK(reactor.main_thread_id != reactor.idle_thread_id);
    CHECK(reactor.main_thread_id != reactor.default_thread_id);
    CHECK(reactor.idle_thread_id == reactor.default_thread_id);

    const std::vector<std::string> expected = {
        "Step<1>",
        "Main Idle",
        "Step<2>",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
