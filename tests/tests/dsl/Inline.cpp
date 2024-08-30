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

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct SimpleMessage {
        SimpleMessage(std::string data) : data(std::move(data)) {}
        std::string data;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<SimpleMessage>, MainThread, Inline::ALWAYS>().then([this](const SimpleMessage& message) {  //
            log_interaction(message.data, "Main Always");
        });
        on<Trigger<SimpleMessage>, MainThread, Inline::NEVER>().then([this](const SimpleMessage& message) {  //
            log_interaction(message.data, "Main Never");
        });
        on<Trigger<SimpleMessage>, MainThread>().then([this](const SimpleMessage& message) {  //
            log_interaction(message.data, "Main Neutral");
        });

        on<Trigger<SimpleMessage>, Pool<>, Inline::ALWAYS>().then([this](const SimpleMessage& message) {  //
            log_interaction(message.data, "Default Always");
        });
        on<Trigger<SimpleMessage>, Pool<>, Inline::NEVER>().then([this](const SimpleMessage& message) {  //
            log_interaction(message.data, "Default Never");
        });
        on<Trigger<SimpleMessage>, Pool<>>().then([this](const SimpleMessage& message) {  //
            log_interaction(message.data, "Default Neutral");
        });

        on<Trigger<Step<1>>, MainThread>().then([this] {
            emit(std::make_unique<SimpleMessage>("Main Local"));
            emit<Scope::INLINE>(std::make_unique<SimpleMessage>("Main Inline"));
        });
        on<Trigger<Step<2>>, Pool<>>().then([this] {
            emit(std::make_unique<SimpleMessage>("Default Local"));
            emit<Scope::INLINE>(std::make_unique<SimpleMessage>("Default Inline"));
        });

        on<Startup>().then([this] {
            emit(std::make_unique<Step<1>>());
            emit(std::make_unique<Step<2>>());
        });
    }

    void log_interaction(const std::string& source, const std::string& target) {
        std::lock_guard<std::mutex> lock(mutex);
        const auto& pool       = NUClear::threading::scheduler::Pool::current();
        events[source][target] = pool->descriptor->name;
    }

    /// A vector of events that have happened
    std::mutex mutex;
    std::map<std::string, std::map<std::string, std::string>> events;
};


TEST_CASE("Test the interactions between inline emits and the Inline dsl keyword") {

    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<NUClear::extension::TraceController>();
    plant.emit<NUClear::dsl::word::emit::Inline>(std::make_unique<NUClear::message::BeginTrace>());
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

    std::vector<std::string> actual;
    for (const auto& type : reactor.events) {
        for (const auto& event : type.second) {
            actual.push_back(type.first + " -> " + event.first + " on " + event.second);
        }
    }

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, actual));

    // Check the events fired in order and only those events
    REQUIRE(actual == expected);
}
