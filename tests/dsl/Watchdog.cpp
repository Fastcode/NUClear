/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#include <numeric>

#include "nuclear"

namespace {

NUClear::clock::time_point start;
NUClear::clock::time_point end;
int count = 0;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        start = NUClear::clock::now();

        // Trigger every 10 milliseconds
        on<Watchdog<TestReactor, 10, std::chrono::milliseconds>>().then([this] {

            end = NUClear::clock::now();

            // When our watchdog eventually triggers, shutdown
            powerplant.shutdown();
        });

        on<Every<5, std::chrono::milliseconds>>().then([this] {

            // service the watchdog
            if (++count < 20) {
                emit(std::make_unique<NUClear::message::ServiceWatchdog<TestReactor>>());
            }
        });
    }
};
}

TEST_CASE("Testing the Watchdog Smart Type", "[api][watchdog]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    // Require that at least 100ms has passed (since 20 * 5ms gives 100, and we should be longer than that)
    REQUIRE(end - start > std::chrono::milliseconds(100));
}
