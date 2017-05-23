/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include "nuclear"

namespace {

std::atomic<int> runcount1(0);
std::atomic<int> runcount2(0);

struct SimpleMessage1 {
    int data;
};

struct SimpleMessage2 {
    int data;
};

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Trigger<SimpleMessage1>, Single>().then([this](const SimpleMessage1&) {

            // Increment our run count
            ++runcount1;

            // Emit a message 2
            emit(std::make_unique<SimpleMessage2>());

            // Wait for 10 ms
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            // Emit another message 2
            emit(std::make_unique<SimpleMessage2>());

            // We are finished the test
            powerplant.shutdown();
        });

        on<Trigger<SimpleMessage2>, Single>().then([this](const SimpleMessage2&) { ++runcount2; });

        on<Startup>().then([this]() {

            // Emit two events, only one should run
            emit(std::make_unique<SimpleMessage1>());
            emit(std::make_unique<SimpleMessage1>());
        });
    }
};
}

TEST_CASE("Test that single prevents a second call while one is executing", "[api][precondition][single]") {

    NUClear::PowerPlant::Configuration config;
    // Unless there are at least 2 threads here single makes no sense ;)
    config.thread_count = 2;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    // Require that only 1 run has happened on message 1
    REQUIRE(runcount1 == 1);

    // Require that 2 runs have happened on message 2
    REQUIRE(runcount2 == 2);
}
