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
private:
    struct Loop {
        explicit Loop(int i) : i(i) {}
        int i;
    };

public:
    static constexpr int n_loops = 10000;

    explicit TestReactor(std::unique_ptr<NUClear::Environment> environment)
        : TestBase(std::move(environment), false, std::chrono::seconds(2)) {

        /*
         * Run idle on the default pool, and a task on the main pool.
         * Default should trigger the idle task and put something in the main threads pool.
         * The main thread pool should then run, preventing the global system from being idle.
         * Once it finishes, main should be idle making the whole system idle and triggering a new task.
         *
         * At no point should two idle tasks fire as either the system will be idle or the main thread will be running.
         */

        on<Trigger<Loop>, MainThread>().then([this](const Loop& l) {
            if (l.i < n_loops) {
                main_calls[l.i].fetch_add(1, std::memory_order_relaxed);
            }
            else {
                powerplant.shutdown();
            }
        });
        on<Idle<>, Pool<>, With<Loop>>().then([this](const Loop& l) {
            if (l.i < n_loops) {
                idle_calls[l.i].fetch_add(1, std::memory_order_relaxed);
                emit(std::make_unique<Loop>(l.i + 1));
            }
        });

        on<Startup>().then([this] { emit(std::make_unique<Loop>(0)); });
    }

    std::array<std::atomic<int>, n_loops> main_calls{};
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

    // Convert the values to a list for comparison
    auto error_points = [&](const std::array<std::atomic<int>, n_loops>& arr) {
        std::map<int, int> hits;
        for (int i = 0; i < n_loops; ++i) {
            if (arr[i] != 1) {
                hits[i] = arr[i];
            }
        }
        return hits;
    };

    auto main_calls     = error_points(reactor.main_calls);
    auto idle_calls     = error_points(reactor.idle_calls);
    auto expected_calls = std::map<int, int>{};

    // The reactions should have triggered n_loops times
    CHECK(main_calls == expected_calls);
    CHECK(idle_calls == expected_calls);
}
