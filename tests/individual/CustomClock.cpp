/*
 * MIT License
 *
 * Copyright (c) 2018 NUClear Contributors
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

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

// This define declares that we are using a custom clock and it should try to link
#define NUCLEAR_CUSTOM_CLOCK
#include <nuclear>

#include "test_util/TestBase.hpp"

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
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

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::vector<std::pair<std::chrono::steady_clock::time_point, NUClear::clock::time_point>> times;

class TestReactor : public test_util::TestBase<TestReactor, 10000> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        // Collect steady clock times as well as NUClear clock times
        on<Every<10, std::chrono::milliseconds>>().then([] {  //
            times.emplace_back(std::chrono::steady_clock::now(), NUClear::clock::now());
        });

        // Collect until the watchdog times out
        on<Watchdog<TestReactor, 1, std::chrono::seconds>>().then([this] {  //
            powerplant.shutdown();
        });
    }
};
}  // namespace

TEST_CASE("Testing custom clock works correctly", "[api][custom_clock]") {

    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    // Calculate the average ratio delta time for steady and custom clocks
    double steady_total = 0;
    double custom_total = 0;

    for (int i = 0; i + 1 < int(times.size()); ++i) {
        using namespace std::chrono;  // NOLINT(google-build-using-namespace) fine in function scope
        steady_total += duration_cast<duration<double>>(times[i + 1].first - times[i].first).count();
        custom_total += duration_cast<duration<double>>(times[i + 1].second - times[i].second).count();
    }

    // The ratio should be about 0.5
    REQUIRE((custom_total / steady_total) == Approx(0.5));

    // The amount of time that passed should be (n - 1) * 2 * 10ms
    REQUIRE(steady_total == Approx(2.0 * (times.size() - 1) * 1e-2).margin(1e-3));
}
