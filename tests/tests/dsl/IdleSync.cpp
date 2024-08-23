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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    explicit TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Trigger<Step<1>>, MainThread>().then([this] {
            emit(std::make_unique<Step<2>>());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            emit(std::make_unique<Step<3>>());
        });

        // Idle testing for default thread
        on<Trigger<Step<2>>, Sync<TestReactor>>().then([] {
            events.push_back("Default Start");
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            events.push_back("Default End");
        });

        on<Trigger<Step<3>>, Sync<TestReactor>, MainThread>().then([] { events.push_back("Main Task"); });

        on<Idle<MainThread>>().then([this] {
            events.push_back("Idle Main Thread");
            powerplant.shutdown();
        });

        on<Startup>().then([this] { emit(std::make_unique<Step<1>>()); });
    }

    /// A vector of events that have happened
    std::vector<std::string> events;
};


TEST_CASE("Test that pool idle triggers when a waiting task prevents running", "[api][idle]") {

    NUClear::Configuration config;
    config.thread_count = 4;
    NUClear::PowerPlant plant(config);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Default Start",
        "Idle Main Thread",
        "Default End",
        "Main Task",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
