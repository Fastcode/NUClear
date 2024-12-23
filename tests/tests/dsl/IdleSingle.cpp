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
#include "util/precise_sleep.hpp"

namespace Catch {

template <typename K, typename V>
struct StringMaker<std::pair<const K, V>> {
    static std::string convert(const std::pair<const K, V>& entry) {
        std::ostringstream oss;
        oss << entry.first << ":" << entry.second;
        return oss.str();
    }
};

}  // namespace Catch


class TestReactor : public test_util::TestBase<TestReactor> {
public:
    static constexpr int n_loops = 250;

    struct TaskB {
        explicit TaskB(int i) : i(i) {}
        int i;
    };
    struct TaskA {
        explicit TaskA(int i) : i(i) {}
        int i;
    };

    struct IdlePool {
        static constexpr int concurrency = 1;
    };

    explicit TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        /*
         * Runs a sync task so that a task can be triggered while the pool is idle.
         * Since the pool can't pick up this task it should remain idle and not fire the idle task again.
         */

        // Entry task for each test
        on<Trigger<TaskA>, Pool<>, Sync<TestReactor>>().then([this](const TaskA& t) {
            entry_calls[t.i].fetch_add(1, std::memory_order_relaxed);
            emit(std::make_unique<TaskB>(t.i));
            NUClear::util::precise_sleep(std::chrono::milliseconds(1));
        });

        // Run this at low priority but have it first
        // This way MainThread will get notified that it has access to Sync but then it will lose it when
        // The other task on the default pool gets created so it'll be notified but unable to act
        on<Trigger<TaskB>, MainThread, Priority::LOW, Sync<TestReactor>>().then([this](const TaskB& t) {
            main_calls[t.i].fetch_add(1, std::memory_order_relaxed);

            if (t.i + 1 < n_loops) {
                emit(std::make_unique<TaskA>(t.i + 1));
            }
            else {
                powerplant.shutdown();
            }
        });
        // This is the high priority task that preempts the main thread and makes it go idle again
        on<Trigger<TaskB>, Pool<>, Priority::HIGH, Sync<TestReactor>>().then([this](const TaskB& t) {  //
            default_calls[t.i].fetch_add(1, std::memory_order_relaxed);
        });

        on<Idle<MainThread>, Pool<IdlePool>, With<TaskA>>().then([this](const TaskA& t) {
            // The main thread should go idle only one time per loop
            idle_calls[t.i].fetch_add(1, std::memory_order_relaxed);
        });

        on<Startup>().then([this] { emit(std::make_unique<TaskA>(0)); });
    }

    std::array<std::atomic<int>, n_loops> entry_calls{};
    std::array<std::atomic<int>, n_loops> main_calls{};
    std::array<std::atomic<int>, n_loops> default_calls{};
    std::array<std::atomic<int>, n_loops> idle_calls{};
};


TEST_CASE("Test that when a global idle trigger exists it is triggered only once", "[api][dsl][Idle][Sync]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    constexpr int n_loops = TestReactor::n_loops;
    std::array<std::atomic<int>, n_loops> expected{};
    for (int i = 0; i < n_loops; ++i) {
        expected[i].store(1, std::memory_order_relaxed);
    }


    // Convert the values to strings for easier comparison
    auto error_points = [&](const std::array<std::atomic<int>, n_loops>& arr) {
        std::map<int, int> hits;
        for (int i = 0; i < n_loops; ++i) {
            if (arr[i] != 1) {
                hits[i] = arr[i];
            }
        }
        return hits;
    };

    auto entry_calls    = error_points(reactor.entry_calls);
    auto default_calls  = error_points(reactor.default_calls);
    auto main_calls     = error_points(reactor.main_calls);
    auto idle_calls     = error_points(reactor.idle_calls);
    auto expected_calls = std::map<int, int>{};

    // The reactions should have triggered n_loops times
    CHECK(entry_calls == expected_calls);
    CHECK(default_calls == expected_calls);
    CHECK(main_calls == expected_calls);
    CHECK(idle_calls == expected_calls);
}
