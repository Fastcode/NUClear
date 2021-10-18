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
#include <iostream>
#include <nuclear>
#include <numeric>
#include <string>

namespace {

NUClear::clock::time_point start;
NUClear::clock::time_point end;
NUClear::clock::time_point end_a;
NUClear::clock::time_point end_b;
int count = 0;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        start = NUClear::clock::now();
        count = 0;

        // Trigger every 10 milliseconds
        on<Watchdog<TestReactor, 10, std::chrono::milliseconds>>().then([this] {
            end = NUClear::clock::now();

            // When our watchdog eventually triggers, shutdown
            powerplant.shutdown();
        });

        on<Every<5, std::chrono::milliseconds>>().then([this] {
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

        // Trigger every 10 milliseconds
        on<Watchdog<TestReactorRuntimeArg, 10, std::chrono::milliseconds>>(std::string("test a")).then([this] {
            end_a = NUClear::clock::now();

            // When our watchdog eventually triggers, shutdown
            powerplant.shutdown();
        });

        on<Watchdog<TestReactorRuntimeArg, 10, std::chrono::milliseconds>>(std::string("test b")).then([this] {
            end_b = NUClear::clock::now();

            // When our watchdog eventually triggers, shutdown
            powerplant.shutdown();
        });

        on<Every<5, std::chrono::milliseconds>>().then([this] {
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

    auto elapsed = end - start;
    std::cout << "[api][watchdog] elapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
              << std::endl;

    // Require that at least 100ms has passed (since 20 * 5ms gives 100, and we should be longer than that)
    REQUIRE(elapsed > std::chrono::milliseconds(100));
}

TEST_CASE("Testing the Watchdog Smart Type with a sub type", "[api][watchdog][sub_type]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactorRuntimeArg>();

    plant.start();

    auto elapsed_a = end_a - start;
    auto elapsed_b = end_b - start;

    std::cout << "[api][watchdog][sub_type] elapsed_a: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_a).count() << std::endl;
    std::cout << "[api][watchdog][sub_type] elapsed_b: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_b).count() << std::endl;

    // Require that at least 100ms has passed (since 20 * 5ms gives 100, and we should be longer than that)
    REQUIRE(elapsed_a > std::chrono::milliseconds(100));
    REQUIRE(elapsed_b > std::chrono::milliseconds(100));
}
