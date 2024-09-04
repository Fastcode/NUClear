/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#include "test_util/TestBase.hpp"
#include "test_util/TimeUnit.hpp"
#include "test_util/common.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct SimpleMessage {
        SimpleMessage(int data) : data(data) {}
        int data;
    };

    template <int N>
    struct CustomPool {
        static constexpr int concurrency = 2;
    };

    template <int N>
    void do_step(const std::string& name) {
        std::this_thread::sleep_until(start_time + test_util::TimeUnit(N));

        const std::lock_guard<std::mutex> lock(events_mutex);
        events.push_back(name + " " + std::to_string(N));
        emit(std::make_unique<Step<N + 1>>());
    }

    explicit TestReactor(std::unique_ptr<NUClear::Environment> environment)
        : TestBase(std::move(environment), false, std::chrono::seconds(5)) {

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
        on<Trigger<Step<16>>, MainThread>().then([this] { mrh.unbind(); });

        // Idle testing for custom pool
        on<Trigger<Step<17>>, Pool<CustomPool<1>>>().then([this] { do_step<17>("Custom<1> Startup"); });
        on<Trigger<Step<18>>, Pool<CustomPool<1>>>().then([this] { do_step<18>("Custom<1> Step"); });
        on<Trigger<Step<19>>, Pool<CustomPool<1>>>().then([this] { do_step<19>("Custom<1> Step"); });
        crh = on<Idle<Pool<CustomPool<1>>>>().then([this] { do_step<20>("Custom<1> Idle"); });
        on<Trigger<Step<21>>, Pool<CustomPool<1>>>().then([this] { do_step<21>("Custom<1> Step"); });
        on<Trigger<Step<22>>, Pool<CustomPool<1>>>().then([this] { do_step<22>("Custom<1> Step"); });
        on<Trigger<Step<23>>, Pool<CustomPool<1>>>().then([this] { do_step<23>("Custom<1> Step"); });
        on<Trigger<Step<24>>, Pool<CustomPool<1>>>().then([this] { crh.unbind(); });

        // Idle testing for global
        on<Trigger<Step<25>>, Pool<CustomPool<2>>>().then([this] { do_step<25>("Custom<2> Startup"); });
        on<Trigger<Step<26>>, Pool<CustomPool<2>>>().then([this] { do_step<26>("Custom<2> Step"); });
        on<Trigger<Step<27>>, Pool<CustomPool<2>>>().then([this] { do_step<27>("Custom<2> Step"); });
        grh = on<Idle<>>().then([this] { do_step<28>("Global Idle"); });
        on<Trigger<Step<29>>, Pool<CustomPool<2>>>().then([this] { do_step<29>("Custom<2> Step"); });
        on<Trigger<Step<30>>, Pool<CustomPool<2>>>().then([this] { do_step<30>("Custom<2> Step"); });
        on<Trigger<Step<31>>, Pool<CustomPool<2>>>().then([this] { do_step<31>("Custom<2> Step"); });
        on<Trigger<Step<32>>, Pool<CustomPool<2>>>().then([this] { powerplant.shutdown(); });

        on<Startup>().then([this] {
            emit(std::make_unique<Step<1>>());
            emit(std::make_unique<Step<9>>());
            emit(std::make_unique<Step<17>>());
            emit(std::make_unique<Step<25>>());
        });
    }

    /// A mutex to protect the events vector
    std::mutex events_mutex;
    /// A vector of events that have happened
    std::vector<std::string> events;

private:
    NUClear::clock::time_point start_time;
    NUClear::threading::ReactionHandle drh;
    NUClear::threading::ReactionHandle mrh;
    NUClear::threading::ReactionHandle crh;
    NUClear::threading::ReactionHandle grh;
};


TEST_CASE("Test that pool idle triggers when nothing is running", "[api][idle]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 4;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Default Startup 1", "Default Step 2",       "Default Step 3",    "Default Idle 4",    "Default Step 5",
        "Default Step 6",    "Default Step 7",       "Main Startup 9",    "Main Step 10",      "Main Step 11",
        "Main Idle 12",      "Main Step 13",         "Main Step 14",      "Main Step 15",      "Custom<1> Startup 17",
        "Custom<1> Step 18", "Custom<1> Step 19",    "Custom<1> Idle 20", "Custom<1> Step 21", "Custom<1> Step 22",
        "Custom<1> Step 23", "Custom<2> Startup 25", "Custom<2> Step 26", "Custom<2> Step 27", "Global Idle 28",
        "Custom<2> Step 29", "Custom<2> Step 30",    "Custom<2> Step 31",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
