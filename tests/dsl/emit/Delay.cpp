/*
 * MIT License
 *
 * Copyright (c) 2016 NUClear Contributors
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

#include "../../test_util/TestBase.hpp"

// Anonymous namespace to keep everything file local
namespace {

/// @brief Events that occur during the test
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/// @brief Test units are the time units the test is performed in
using TestUnits = std::chrono::duration<int64_t, std::ratio<1, 20>>;
/// @brief Perform this many different time points for the test
constexpr int test_loops = 5;

struct DelayedMessage {
    DelayedMessage(const NUClear::clock::duration& delay) : time(NUClear::clock::now()), delay(delay) {}
    NUClear::clock::time_point time;
    NUClear::clock::duration delay;
};

struct TargetTimeMessage {
    TargetTimeMessage(const NUClear::clock::time_point& target) : time(NUClear::clock::now()), target(target) {}
    NUClear::clock::time_point time;
    NUClear::clock::time_point target;
};

struct FinishTest {};

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        // Measure when messages were sent and received and print those values
        on<Trigger<DelayedMessage>>().then([](const DelayedMessage& m) {
            auto true_delta = std::chrono::duration_cast<TestUnits>(NUClear::clock::now() - m.time);
            auto delta      = std::chrono::duration_cast<TestUnits>(m.delay);

            // Print the debug message
            events.push_back("delayed " + std::to_string(true_delta.count()) + " received "
                             + std::to_string(delta.count()));
        });

        on<Trigger<TargetTimeMessage>>().then([](const TargetTimeMessage& m) {
            auto true_delta = std::chrono::duration_cast<TestUnits>(NUClear::clock::now() - m.time);
            auto delta      = std::chrono::duration_cast<TestUnits>(m.target - m.time);

            // Print the debug message
            events.push_back("at_time " + std::to_string(true_delta.count()) + " received "
                             + std::to_string(delta.count()));
        });

        on<Trigger<FinishTest>>().then([this] {
            events.push_back("Finished");
            powerplant.shutdown();
        });


        on<Startup>().then([this] {
            // Get our jump size in milliseconds
            const int jump_unit = (TestUnits::period::num * 1000) / TestUnits::period::den;
            // Delay with consistent jumps
            for (int i = 0; i < test_loops; ++i) {
                auto delay = std::chrono::milliseconds(jump_unit * i);
                emit<Scope::DELAY>(std::make_unique<DelayedMessage>(delay), delay);
            }

            // Target time with consistent jumps that interleave the first set
            for (int i = 0; i < test_loops; ++i) {
                auto target = NUClear::clock::now() + std::chrono::milliseconds(jump_unit / 2 + jump_unit * i);
                emit<Scope::DELAY>(std::make_unique<TargetTimeMessage>(target), target);
            }

            // Emit a shutdown one time unit after
            emit<Scope::DELAY>(std::make_unique<FinishTest>(), std::chrono::milliseconds(jump_unit * (test_loops + 1)));
        });
    }
};
}  // namespace

TEST_CASE("Testing the delay emit", "[api][emit][delay]") {
    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "delayed 0 received 0",
        "at_time 0 received 0",
        "delayed 1 received 1",
        "at_time 1 received 1",
        "delayed 2 received 2",
        "at_time 2 received 2",
        "delayed 3 received 3",
        "at_time 3 received 3",
        "delayed 4 received 4",
        "at_time 4 received 4",
        "Finished",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
