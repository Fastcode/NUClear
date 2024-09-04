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
    template <int i>
    struct Message {
        int val;
        Message(int val) : val(val) {};
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<Message<1>>, With<Message<2>>, With<Message<3>>, With<Message<4>>>().then(
            [this](const Message<2>& m2, const Message<4>& m4) {
                events.push_back("Message<2>: " + std::to_string(m2.val));
                events.push_back("Message<4>: " + std::to_string(m4.val));
            });

        on<Startup>().then([this] {
            // Emit from message 4 to 1
            events.push_back("Emitting Message<4>");
            emit(std::make_unique<Message<4>>(4 * 4));
            events.push_back("Emitting Message<3>");
            emit(std::make_unique<Message<3>>(3 * 3));
            events.push_back("Emitting Message<2>");
            emit(std::make_unique<Message<2>>(2 * 2));
            events.push_back("Emitting Message<1>");
            emit(std::make_unique<Message<1>>(1 * 1));
        });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Testing that when arguments missing from the call it can still run", "[api][missing_arguments]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Emitting Message<4>",
        "Emitting Message<3>",
        "Emitting Message<2>",
        "Emitting Message<1>",
        "Message<2>: 4",
        "Message<4>: 16",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
