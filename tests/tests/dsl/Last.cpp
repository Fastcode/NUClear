/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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
#include "test_util/common.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct TestMessage {
        TestMessage(int v) : value(v) {};
        int value;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Last<5, Trigger<TestMessage>>>().then([this](std::list<std::shared_ptr<const TestMessage>> messages) {
            std::stringstream ss;
            for (auto& m : messages) {
                ss << m->value << " ";
            }
            events.push_back(ss.str());

            // Finish when we get to 10
            if (messages.back()->value < 10) {
                // Send out another message
                emit(std::make_unique<TestMessage>(messages.back()->value + 1));
            }
        });

        on<Startup>().then([this] { emit(std::make_unique<TestMessage>(0)); });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Testing the last n feature", "[api][last]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "0 ",
        "0 1 ",
        "0 1 2 ",
        "0 1 2 3 ",
        "0 1 2 3 4 ",
        "1 2 3 4 5 ",
        "2 3 4 5 6 ",
        "3 4 5 6 7 ",
        "4 5 6 7 8 ",
        "5 6 7 8 9 ",
        "6 7 8 9 10 ",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
