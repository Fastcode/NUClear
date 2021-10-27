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

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

// This define declares that we are using a custom clock and it should try to link
#define NUCLEAR_CUSTOM_CLOCK
#include <nuclear>

std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

namespace NUClear {
clock::time_point clock::now() {

    // Add half the time since we started (time moving at half speed)
    auto now = std::chrono::steady_clock::now();
    return clock::time_point(start + (now - start) / 2);
}
}  // namespace NUClear

// Anonymous namespace to keep everything file local
namespace {

template <int id>
struct Message {};

std::vector<std::chrono::steady_clock::time_point> times;
constexpr int n_time = 100;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Running every this slowed down clock should execute slower
        on<Every<10, std::chrono::milliseconds>>().then([this] {
            times.push_back(std::chrono::steady_clock::now());
            if (times.size() > n_time) { powerplant.shutdown(); }
        });
    }
};
}  // namespace

TEST_CASE("Testing custom clock works correctly", "[api][custom_clock]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);

    // We are installing with an initial log level of debug
    plant.install<TestReactor>();

    plant.start();

    // Build up our difference vector
    double total = 0;
    for (size_t i = 0; i < times.size() - 1; ++i) {
        total += (double((times[i + 1] - times[i]).count()) / double(std::nano::den));
    }

#ifdef _WIN32
    double timing_epsilon = 1e-2;
#else
    double timing_epsilon = 1e-3;
#endif

    // The total should be about 2.0
    REQUIRE(total == Approx(2.0).epsilon(timing_epsilon));
}
