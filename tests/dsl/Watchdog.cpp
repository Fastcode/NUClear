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
#include <nuclear>
#include <numeric>
#include <string>

namespace {

NUClear::clock::time_point start;
NUClear::clock::time_point end;
NUClear::clock::time_point end_a;
NUClear::clock::time_point end_b;
bool a_ended = false;
bool b_ended = false;

int count = 0;

#ifdef _WIN32
// The precision of timing on Windows (with the current NUClear timing method) is not great.
// This defines the intervals larger to avoid the problems at smaller intervals
// TODO(Josephus or Trent): use a higher precision timing method on Windows (look into nanosleep,
// in addition to the condition lock and spin lock used in ChronoController.hpp)
constexpr int WATCHDOG_TIMEOUT = 30;
constexpr int EVERY_INTERVAL   = 5;
#else
constexpr int WATCHDOG_TIMEOUT = 10;
constexpr int EVERY_INTERVAL   = 5;
#endif

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        start = NUClear::clock::now();
        count = 0;

        // Trigger the watchdog after WATCHDOG_TIMEOUT milliseconds
        on<Watchdog<TestReactor, WATCHDOG_TIMEOUT, std::chrono::milliseconds>>().then([this] {
            end = NUClear::clock::now();

            // When our watchdog eventually triggers, shutdown
            powerplant.shutdown();
        });

        // Service the watchdog every EVERY_INTERVAL milliseconds, 20 times. Then let it expire to trigger and end the
        // test.
        on<Every<EVERY_INTERVAL, std::chrono::milliseconds>>().then([this] {
            // service the watchdog
            if (++count < 20) { emit<Scope::WATCHDOG>(ServiceWatchdog<TestReactor>()); }
        });
    }
};

class TestReactorRuntimeArg : public NUClear::Reactor {
public:
    TestReactorRuntimeArg(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        start = NUClear::clock::now();
        count = 0;

        // Trigger the watchdog after WATCHDOG_TIMEOUT milliseconds
        on<Watchdog<TestReactorRuntimeArg, WATCHDOG_TIMEOUT, std::chrono::milliseconds>>(std::string("test a"))
            .then([this] {
                end_a   = NUClear::clock::now();
                a_ended = true;

                // When our watchdog eventually triggers, shutdown
                if (b_ended) { powerplant.shutdown(); }
            });

        // Trigger the watchdog after WATCHDOG_TIMEOUT milliseconds
        on<Watchdog<TestReactorRuntimeArg, WATCHDOG_TIMEOUT, std::chrono::milliseconds>>(std::string("test b"))
            .then([this] {
                end_b   = NUClear::clock::now();
                b_ended = true;

                // When our watchdog eventually triggers, shutdown
                if (a_ended) { powerplant.shutdown(); }
            });

        // Service the watchdog every EVERY_INTERVAL milliseconds, 20 times. Then let it expire to trigger and end the
        // test.
        on<Every<EVERY_INTERVAL, std::chrono::milliseconds>>().then([this] {
            // service the watchdog
            if (++count < 20) {
                emit<Scope::WATCHDOG>(ServiceWatchdog<TestReactorRuntimeArg>(std::string("test a")));
                emit<Scope::WATCHDOG>(ServiceWatchdog<TestReactorRuntimeArg>(std::string("test b")));
            }
        });
    }
};
}  // namespace

TEST_CASE("Testing the Watchdog Smart Type", "[api][watchdog]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    // Require that at least the minimum time interval to have run all Everys has passed
    REQUIRE(end - start > std::chrono::milliseconds(20 * EVERY_INTERVAL));
}

TEST_CASE("Testing the Watchdog Smart Type with a sub type", "[api][watchdog][sub_type]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactorRuntimeArg>();

    plant.start();

    // Require that at least the minimum time interval to have run all Everys has passed
    REQUIRE(end_a - start > std::chrono::milliseconds(20 * EVERY_INTERVAL));
    REQUIRE(end_b - start > std::chrono::milliseconds(20 * EVERY_INTERVAL));
}
