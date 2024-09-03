/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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
#include <vector>

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    static constexpr int N_EVENTS = 1000;

    template <char c>
    struct Message {
        int val;
        Message(int val) : val(val) {};
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<Message<'A'>>, Sync<TestReactor>>().then([this](const Message<'A'>& m) {  //
            events.emplace_back('A', m.val);
        });

        on<Trigger<Message<'B'>>, Sync<TestReactor>, MainThread>().then([this](const Message<'B'>& m) {  //
            events.emplace_back('B', m.val);
        });

        on<Startup>().then("Startup", [this] {
            for (int i = 0; i < N_EVENTS; ++i) {
                emit(std::make_unique<Message<'A'>>(i));
                emit(std::make_unique<Message<'B'>>(i));
            }
        });
    }

    /// Events that occur during the test
    std::vector<std::pair<char, int>> events;
};


TEST_CASE("Sync events execute in order", "[api][sync][priority]") {
    NUClear::Configuration config;
    config.default_pool_concurrency = 4;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    // Expect interleaved A and B events in order
    std::vector<std::pair<char, int>> expected(reactor.events.size());
    for (int i = 0; i < TestReactor::N_EVENTS; ++i) {
        expected[2 * i]     = {'A', i};
        expected[2 * i + 1] = {'B', i};
    }

    REQUIRE(reactor.events == expected);
}
