/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
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

/// @brief A vector of events that have happened
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

class TestReactor : public test_util::TestBase<TestReactor, 1000> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        // Idle testing for default thread
        on<Trigger<Step<1>>, Sync<TestReactor>>().then([this] {
            events.push_back("Step 1 Start");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            events.push_back("Step 1 End");
        });
        on<Trigger<Step<2>>, Sync<TestReactor>, MainThread>().then([this] { events.push_back("Step 2"); });
        on<Idle<MainThread>>().then([this] {
            events.push_back("Idle Main Thread");
            powerplant.shutdown();
        });

        on<Startup>().then([this] {
            emit(std::make_unique<Step<1>>());
            emit(std::make_unique<Step<2>>());
        });
    }
};

}  // namespace

TEST_CASE("Test that pool idle triggers when a waiting task prevents running", "[api][idle]") {

    NUClear::Configuration config;
    config.thread_count = 4;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Step 1 Start",
        "Idle Main Thread",
        "Step 1 End",
        "Step 2",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
