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
    struct Message {
        Message(int i) : i(i) {}
        int i;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        // Make an always disabled reaction
        a = on<Trigger<Message>, Priority::HIGH>().then([this](const Message& msg) {  //
            events.push_back("Executed disabled reaction " + std::to_string(msg.i));
        });
        a.disable();

        // Make a reaction that we toggle on and off
        b = on<Trigger<Message>, Priority::HIGH>().then([this](const Message& msg) {  //
            events.push_back("Executed toggled reaction " + std::to_string(msg.i));
            b.disable();
            emit(std::make_unique<Message>(1));
        });

        on<Trigger<Message>>().then([this](const Message& msg) {  //
            events.push_back("Executed enabled reaction " + std::to_string(msg.i));
        });

        // Start our test
        on<Startup>().then([this] {  //
            emit(std::make_unique<Message>(0));
        });
    }

    ReactionHandle a;
    ReactionHandle b;

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Testing reaction handle functionality", "[api][reactionhandle]") {
    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Executed toggled reaction 0",
        "Executed enabled reaction 0",
        "Executed enabled reaction 1",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
