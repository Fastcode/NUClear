/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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
        Message(std::string data) : data(std::move(data)) {}
        std::string data;
    };
    struct Data {
        Data(std::string data) : data(std::move(data)) {}
        std::string data;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {
        // Check that the lists are combined, and that the function args are in order
        on<Trigger<Message>, With<Data>>().then([this](const Message& m, const Data& d) {  //
            events.push_back("Message: " + m.data + " Data: " + d.data);
        });

        on<Trigger<Step<1>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Data 1");
            emit(std::make_unique<Data>("D1"));
        });

        on<Trigger<Step<2>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Data 2");
            emit(std::make_unique<Data>("D2"));
        });

        on<Trigger<Step<3>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Message 1");
            emit(std::make_unique<Message>("M1"));
        });

        on<Trigger<Step<4>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Data 3");
            emit(std::make_unique<Data>("D3"));
        });

        on<Trigger<Step<5>>, Priority::LOW>().then([this] {
            events.push_back("Emitting Message 2");
            emit(std::make_unique<Message>("M2"));
        });

        on<Startup>().then([this] {
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


TEST_CASE("Testing the with dsl keyword", "[api][with]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Emitting Data 1",
        "Emitting Data 2",
        "Emitting Message 1",
        "Message: M1 Data: D2",
        "Emitting Data 3",
        "Emitting Message 2",
        "Message: M2 Data: D3",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
