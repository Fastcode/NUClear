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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct SimpleMessage {
        SimpleMessage(int data) : data(data) {}
        int data;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<SimpleMessage>>().then([this](const SimpleMessage& message) {  //
            events.push_back("Trigger " + std::to_string(message.data));
        });

        on<Startup>().then([this] {
            // Emit some messages with data
            for (int i = 0; i < 10; ++i) {
                emit(std::make_unique<SimpleMessage>(i));
            }
        });
    }

    /// A vector of events that have happened
    std::vector<std::string> events;
};


TEST_CASE("Test that Trigger statements get the correct data", "[api][trigger]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Trigger 0",
        "Trigger 1",
        "Trigger 2",
        "Trigger 3",
        "Trigger 4",
        "Trigger 5",
        "Trigger 6",
        "Trigger 7",
        "Trigger 8",
        "Trigger 9",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
