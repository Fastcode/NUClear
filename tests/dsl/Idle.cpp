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

struct SimpleMessage {
    SimpleMessage(int data) : data(data) {}
    int data;
};

constexpr int time_step = 100;

class TestReactor : public test_util::TestBase<TestReactor, 10000> {
public:
    struct CustomPool {
        static constexpr int thread_count = 2;
    };

    template <int N>
    void do_step(const std::string& name) {
        std::this_thread::sleep_until(start_time + std::chrono::milliseconds(time_step * N));
        events.push_back(name + " " + std::to_string(N));
        emit(std::make_unique<Step<N + 1>>());
    }

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        start_time = NUClear::clock::now();

        // Idle testing for default thread
        on<Trigger<Step<1>>>().then([this] { do_step<1>("Default Startup"); });
        on<Trigger<Step<2>>>().then([this] { do_step<2>("Default Step"); });
        on<Trigger<Step<3>>>().then([this] { do_step<3>("Default Step"); });
        drh = on<Idle<Pool<>>>().then([this] { do_step<4>("Default Idle"); });
        on<Trigger<Step<5>>>().then([this] { do_step<5>("Default Step"); });
        on<Trigger<Step<6>>>().then([this] { do_step<6>("Default Step"); });
        on<Trigger<Step<7>>>().then([this] { do_step<7>("Default Step"); });
        on<Trigger<Step<8>>>().then([this] { drh.unbind(); });

        // Idle testing for main thread
        on<Trigger<Step<9>>, MainThread>().then([this] { do_step<9>("Main Startup"); });
        on<Trigger<Step<10>>, MainThread>().then([this] { do_step<10>("Main Step"); });
        on<Trigger<Step<11>>, MainThread>().then([this] { do_step<11>("Main Step"); });
        mrh = on<Idle<MainThread>>().then([this] { do_step<12>("Main Idle"); });
        on<Trigger<Step<13>>, MainThread>().then([this] { do_step<13>("Main Step"); });
        on<Trigger<Step<14>>, MainThread>().then([this] { do_step<14>("Main Step"); });
        on<Trigger<Step<15>>, MainThread>().then([this] { do_step<15>("Main Step"); });
        on<Trigger<Step<15>>, MainThread>().then([this] { mrh.unbind(); });

        // Idle testing for custom pool
        on<Trigger<Step<16>>, Pool<CustomPool>>().then([this] { do_step<16>("Custom Startup"); });
        on<Trigger<Step<17>>, Pool<CustomPool>>().then([this] { do_step<17>("Custom Step"); });
        on<Trigger<Step<18>>, Pool<CustomPool>>().then([this] { do_step<18>("Custom Step"); });
        crh = on<Idle<Pool<CustomPool>>>().then([this] { do_step<19>("Custom Idle"); });
        on<Trigger<Step<20>>, Pool<CustomPool>>().then([this] { do_step<20>("Custom Step"); });
        on<Trigger<Step<21>>, Pool<CustomPool>>().then([this] { do_step<21>("Custom Step"); });
        on<Trigger<Step<22>>, Pool<CustomPool>>().then([this] { do_step<22>("Custom Step"); });
        on<Trigger<Step<23>>, Pool<CustomPool>>().then([this] { crh.unbind(); });

        // Global idle everything finished
        on<Idle<>>().then([this] {
            events.push_back("Global Idle");
            powerplant.shutdown();
        });

        on<Startup>().then([this] {
            emit(std::make_unique<Step<1>>());
            emit(std::make_unique<Step<9>>());
            emit(std::make_unique<Step<16>>());
        });
    }

private:
    NUClear::clock::time_point start_time;
    NUClear::threading::ReactionHandle drh;
    NUClear::threading::ReactionHandle mrh;
    NUClear::threading::ReactionHandle crh;
};

}  // namespace


TEST_CASE("Test that pool idle triggers when nothing is running", "[api][idle]") {

    NUClear::Configuration config;
    config.thread_count = 4;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Startup 0",
        "Step 1",
        "Step 2",
        "Step 3",
        "Global Idle 4",
        "Step 5",
        "Step 6",
        "Step 7",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
