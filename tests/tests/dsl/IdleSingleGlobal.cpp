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

namespace {

constexpr int n_loops = 100;

struct Loop {
    Loop(int i) : i(i) {}
    int i;
};

struct IdlePool {
    static constexpr int thread_count = 4;
};

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        // Bounce between the main and default thread to try to get idle to fire when it shouldn't
        on<Trigger<Loop>, MainThread>().then([this](const Loop& l) {
            main_calls[l.i].fetch_add(1, std::memory_order_relaxed);
            if (l.i >= n_loops) {
                powerplant.shutdown();
            }
        });
        on<Idle<>, With<Loop>>().then([this](const Loop& l) {
            idle_calls[l.i].fetch_add(1, std::memory_order_relaxed);
            emit(std::make_unique<Loop>(l.i + 1));
        });


        on<Startup>().then([this] { emit(std::make_unique<Loop>(0)); });
    }

    std::array<std::atomic<int>, n_loops> main_calls{};
    std::array<std::atomic<int>, n_loops> idle_calls{};
};

}  // namespace


TEST_CASE("Test that when a global idle trigger exists it is triggered only once", "[api][dsl][Idle][Sync]") {

    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    auto& reactor = plant.install<TestReactor>();
    plant.start();

    std::array<std::atomic<int>, n_loops> expected{};
    for (int i = 0; i < n_loops; ++i) {
        expected[i].store(1, std::memory_order_relaxed);
    }

    // Convert the values to strings for easier comparison
    auto to_string = [](const std::array<std::atomic<int>, n_loops>& arr) {
        std::string str;
        for (int i = 0; i < n_loops; ++i) {
            str += arr[i] == 0 ? "-" : arr[i] == 1 ? " " : "X";
        }
        return str;
    };

    std::string main_calls     = to_string(reactor.main_calls);
    std::string idle_calls     = to_string(reactor.idle_calls);
    std::string expected_calls = to_string(expected);

    // The reactions should have triggered n_loops times
    CHECK(main_calls == expected_calls);
    CHECK(idle_calls == expected_calls);
}
