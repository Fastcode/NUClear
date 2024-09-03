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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>
#include <numeric>
#include <string>

#include "test_util/TestBase.hpp"
#include "test_util/TimeUnit.hpp"
#include "test_util/common.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    template <int I>
    struct Flag {};

    TestReactor(std::unique_ptr<NUClear::Environment> environment)
        : TestBase(std::move(environment), false, test_util::TimeUnit(40)), start(NUClear::clock::now()) {

        on<Watchdog<Flag<1>, 5, test_util::TimeUnit>>().then([this] {
            events.push_back("Watchdog 1  triggered @ " + units_since_start());
            powerplant.shutdown();
        });

        on<Watchdog<Flag<2>, 4, test_util::TimeUnit>>().then([this] {
            if (flag2++ < 3) {
                events.push_back("Watchdog 2  triggered @ " + units_since_start());
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<1>>());
            }
        });

        // Watchdog with subtypes
        on<Watchdog<Flag<3>, 3, test_util::TimeUnit>>('a').then([this] {
            if (flag3a++ < 3) {
                events.push_back("Watchdog 3A triggered @ " + units_since_start());
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<1>>());
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<2>>());
            }
        });
        on<Watchdog<Flag<3>, 2, test_util::TimeUnit>>('b').then([this] {
            if (flag3b++ < 3) {
                events.push_back("Watchdog 3B triggered @ " + units_since_start());
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<1>>());
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<2>>());
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<3>>('a'));
            }
        });

        on<Watchdog<Flag<4>, 1, test_util::TimeUnit>>().then([this] {
            if (flag4++ < 3) {
                events.push_back("Watchdog 4  triggered @ " + units_since_start());
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<1>>());
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<2>>());
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<3>>('a'));
                emit<Scope::WATCHDOG>(ServiceWatchdog<Flag<3>>('b'));
            }
        });
    }

    std::string units_since_start() const {
        return std::to_string(test_util::round_to_test_units(NUClear::clock::now() - start).count());
    }

    NUClear::clock::time_point start;
    int flag2{0};
    int flag3a{0};
    int flag3b{0};
    int flag4{0};

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Testing the Watchdog Smart Type", "[api][watchdog]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<NUClear::extension::ChronoController>();
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Watchdog 4  triggered @ 1",
        "Watchdog 4  triggered @ 2",
        "Watchdog 4  triggered @ 3",
        "Watchdog 3B triggered @ 5",
        "Watchdog 3B triggered @ 7",
        "Watchdog 3B triggered @ 9",
        "Watchdog 3A triggered @ 12",
        "Watchdog 3A triggered @ 15",
        "Watchdog 3A triggered @ 18",
        "Watchdog 2  triggered @ 22",
        "Watchdog 2  triggered @ 26",
        "Watchdog 2  triggered @ 30",
        "Watchdog 1  triggered @ 35",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
