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

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct Message {
        Message(int i) : i(i) {}
        int i;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<Message>>().then([this](const Message& msg) {  //
            events.push_back("Trigger reaction " + std::to_string(msg.i));
        });
        on<Trigger<Message>, Single>().then([this](const Message& msg) {  //
            events.push_back("Single reaction " + std::to_string(msg.i));
        });
        on<Trigger<Message>, Buffer<2>>().then([this](const Message& msg) {  //
            events.push_back("Buffer<2> reaction " + std::to_string(msg.i));
        });
        on<Trigger<Message>, Buffer<3>>().then([this](const Message& msg) {  //
            events.push_back("Buffer<3> reaction " + std::to_string(msg.i));
        });
        on<Trigger<Message>, Buffer<4>>().then([this](const Message& msg) {  //
            events.push_back("Buffer<4> reaction " + std::to_string(msg.i));
        });

        on<Trigger<Step<1>>, Priority::LOW>().then([this] {
            events.push_back("Step 1");
            emit(std::make_unique<Message>(1));
        });
        on<Trigger<Step<2>>, Priority::LOW>().then([this] {
            events.push_back("Step 2");
            emit(std::make_unique<Message>(2));
            emit(std::make_unique<Message>(3));
        });
        on<Trigger<Step<3>>, Priority::LOW>().then([this] {
            events.push_back("Step 3");
            emit(std::make_unique<Message>(4));
            emit(std::make_unique<Message>(5));
            emit(std::make_unique<Message>(6));
        });
        on<Trigger<Step<4>>, Priority::LOW>().then([this] {
            events.push_back("Step 4");
            emit(std::make_unique<Message>(7));
            emit(std::make_unique<Message>(8));
            emit(std::make_unique<Message>(9));
            emit(std::make_unique<Message>(10));
        });
        on<Trigger<Step<5>>, Priority::LOW>().then([this] {
            events.push_back("Step 5");
            emit(std::make_unique<Message>(11));
            emit(std::make_unique<Message>(12));
            emit(std::make_unique<Message>(13));
            emit(std::make_unique<Message>(14));
            emit(std::make_unique<Message>(15));
        });

        on<Startup>().then("Startup", [this]() {
            emit(std::make_unique<Step<1>>());
            emit(std::make_unique<Step<2>>());
            emit(std::make_unique<Step<3>>());
            emit(std::make_unique<Step<4>>());
            emit(std::make_unique<Step<5>>());
        });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Test that Buffer and Single limit the number of concurrent executions", "[api][precondition][single]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Step 1",
        "Trigger reaction 1",
        "Single reaction 1",
        "Buffer<2> reaction 1",
        "Buffer<3> reaction 1",
        "Buffer<4> reaction 1",
        "Step 2",
        "Trigger reaction 2",
        "Single reaction 2",
        "Buffer<2> reaction 2",
        "Buffer<3> reaction 2",
        "Buffer<4> reaction 2",
        "Trigger reaction 3",
        "Buffer<2> reaction 3",
        "Buffer<3> reaction 3",
        "Buffer<4> reaction 3",
        "Step 3",
        "Trigger reaction 4",
        "Single reaction 4",
        "Buffer<2> reaction 4",
        "Buffer<3> reaction 4",
        "Buffer<4> reaction 4",
        "Trigger reaction 5",
        "Buffer<2> reaction 5",
        "Buffer<3> reaction 5",
        "Buffer<4> reaction 5",
        "Trigger reaction 6",
        "Buffer<3> reaction 6",
        "Buffer<4> reaction 6",
        "Step 4",
        "Trigger reaction 7",
        "Single reaction 7",
        "Buffer<2> reaction 7",
        "Buffer<3> reaction 7",
        "Buffer<4> reaction 7",
        "Trigger reaction 8",
        "Buffer<2> reaction 8",
        "Buffer<3> reaction 8",
        "Buffer<4> reaction 8",
        "Trigger reaction 9",
        "Buffer<3> reaction 9",
        "Buffer<4> reaction 9",
        "Trigger reaction 10",
        "Buffer<4> reaction 10",
        "Step 5",
        "Trigger reaction 11",
        "Single reaction 11",
        "Buffer<2> reaction 11",
        "Buffer<3> reaction 11",
        "Buffer<4> reaction 11",
        "Trigger reaction 12",
        "Buffer<2> reaction 12",
        "Buffer<3> reaction 12",
        "Buffer<4> reaction 12",
        "Trigger reaction 13",
        "Buffer<3> reaction 13",
        "Buffer<4> reaction 13",
        "Trigger reaction 14",
        "Buffer<4> reaction 14",
        "Trigger reaction 15",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
