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

/// Events that occur during the test
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

struct Message {
    Message(std::string data) : data(std::move(data)) {}
    std::string data;
};

struct Data {
    Data(std::string data) : data(std::move(data)) {}
    std::string data;
};

/**
 * Test a raw function that takes no arguments and has a return type.
 *
 * The return type should be ignored and this function should run without issue.
 *
 * @return A value
 */
double raw_function_test_no_args() {
    events.push_back("Raw function no args");
    return 5.0;
}

/**
 * Raw function that takes one argument (the left side of the trigger)
 *
 * @param msg The message
 */
void raw_function_test_left_arg(const Message& msg) {
    events.push_back("Raw function left arg: " + msg.data);
}

/**
 * Raw function that takes one argument (the right side of the trigger)
 *
 * @param data The data
 */
void raw_function_test_right_arg(const Data& data) {
    events.push_back("Raw function right arg: " + data.data);
}

/**
 * Raw function that takes both arguments
 *
 * @param msg  The message
 * @param data The data
 */
void raw_function_test_both_args(const Message& msg, const Data& data) {
    events.push_back("Raw function both args: " + msg.data + " " + data.data);
}

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<Message>, Trigger<Data>>().then(raw_function_test_no_args);
        on<Trigger<Message>, Trigger<Data>>().then(raw_function_test_left_arg);
        on<Trigger<Message>, Trigger<Data>>().then(raw_function_test_right_arg);
        on<Trigger<Message>, Trigger<Data>>().then(raw_function_test_both_args);

        on<Trigger<Step<1>>, Priority::LOW>().then([this] { emit(std::make_unique<Data>("D1")); });
        on<Trigger<Step<2>>, Priority::LOW>().then([this] { emit(std::make_unique<Message>("M2")); });
        on<Trigger<Step<3>>, Priority::LOW>().then([this] { emit(std::make_unique<Data>("D3")); });
        on<Trigger<Step<4>>, Priority::LOW>().then([this] { emit(std::make_unique<Message>("M4")); });

        on<Startup>().then([this] {
            emit(std::make_unique<Step<1>>());
            emit(std::make_unique<Step<2>>());
            emit(std::make_unique<Step<3>>());
            emit(std::make_unique<Step<4>>());
        });
    }
};


TEST_CASE("Test reaction can take a raw function instead of just a lambda", "[api][raw_function]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Raw function no args",
        "Raw function left arg: M2",
        "Raw function right arg: D1",
        "Raw function both args: M2 D1",
        "Raw function no args",
        "Raw function left arg: M2",
        "Raw function right arg: D3",
        "Raw function both args: M2 D3",
        "Raw function no args",
        "Raw function left arg: M4",
        "Raw function right arg: D3",
        "Raw function both args: M4 D3",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
