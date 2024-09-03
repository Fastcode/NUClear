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
#include "test_util/TimeUnit.hpp"
#include "test_util/common.hpp"
#include "util/precise_sleep.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct SimpleMessage {
        SimpleMessage(std::string data) : data(std::move(data)) {}
        std::string data;
        std::thread::id emitter = std::this_thread::get_id();
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<SimpleMessage>, MainThread, Inline::ALWAYS>().then([this](const SimpleMessage& message) {  //
            log_interaction(message, "Main Always");
        });
        on<Trigger<SimpleMessage>, MainThread, Inline::NEVER>().then([this](const SimpleMessage& message) {  //
            log_interaction(message, "Main Never");
        });
        on<Trigger<SimpleMessage>, MainThread>().then([this](const SimpleMessage& message) {  //
            log_interaction(message, "Main Neutral");
        });

        on<Trigger<SimpleMessage>, Pool<>, Inline::ALWAYS>().then([this](const SimpleMessage& message) {  //
            log_interaction(message, "Default Always");
        });
        on<Trigger<SimpleMessage>, Pool<>, Inline::NEVER>().then([this](const SimpleMessage& message) {  //
            log_interaction(message, "Default Never");
        });
        on<Trigger<SimpleMessage>, Pool<>>().then([this](const SimpleMessage& message) {  //
            log_interaction(message, "Default Neutral");
        });

        on<Trigger<Step<1>>, MainThread>().then([this] {
            emit(std::make_unique<SimpleMessage>("Main Local"));
            emit<Scope::INLINE>(std::make_unique<SimpleMessage>("Main Inline"));
            NUClear::util::precise_sleep(test_util::TimeUnit(2));  // Sleep for a bit to give other threads a chance
        });
        on<Trigger<Step<2>>, Pool<>>().then([this] {
            emit(std::make_unique<SimpleMessage>("Default Local"));
            emit<Scope::INLINE>(std::make_unique<SimpleMessage>("Default Inline"));
            NUClear::util::precise_sleep(test_util::TimeUnit(2));  // Sleep for a bit to give other threads a chance
        });

        on<Startup>().then([this] {
            emit(std::make_unique<Step<1>>());
            emit(std::make_unique<Step<2>>());
        });
    }

    void log_interaction(const SimpleMessage& source, const std::string& target) {
        const std::lock_guard<std::mutex> lock(mutex);
        const auto& pool = NUClear::threading::scheduler::Pool::current();
        events[source.data][target] =
            pool->descriptor->name + " "
            + (source.emitter == std::this_thread::get_id() ? "same thread" : "different thread");
    }

    /// A vector of events that have happened
    std::mutex mutex;
    std::map<std::string, std::map<std::string, std::string>> events;
};


TEST_CASE("Test the interactions between inline emits and the Inline dsl keyword") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 4;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Default Inline -> Default Always on Default same thread",
        "Default Inline -> Default Neutral on Default same thread",
        "Default Inline -> Default Never on Default different thread",
        "Default Inline -> Main Always on Default same thread",
        "Default Inline -> Main Neutral on Default same thread",
        "Default Inline -> Main Never on Main different thread",
        "Default Local -> Default Always on Default same thread",
        "Default Local -> Default Neutral on Default different thread",
        "Default Local -> Default Never on Default different thread",
        "Default Local -> Main Always on Default same thread",
        "Default Local -> Main Neutral on Main different thread",
        "Default Local -> Main Never on Main different thread",
        "Main Inline -> Default Always on Main same thread",
        "Main Inline -> Default Neutral on Main same thread",
        "Main Inline -> Default Never on Default different thread",
        "Main Inline -> Main Always on Main same thread",
        "Main Inline -> Main Neutral on Main same thread",
        "Main Inline -> Main Never on Main same thread",
        "Main Local -> Default Always on Main same thread",
        "Main Local -> Default Neutral on Default different thread",
        "Main Local -> Default Never on Default different thread",
        "Main Local -> Main Always on Main same thread",
        "Main Local -> Main Neutral on Main same thread",
        "Main Local -> Main Never on Main same thread",
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
