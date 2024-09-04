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
#include "test_util/TimeUnit.hpp"
#include "test_util/common.hpp"
#include "util/precise_sleep.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    template <int i>
    struct Message {
        Message(std::string data) : data(std::move(data)) {};
        std::string data;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<Message<0>>, Sync<TestReactor>>().then([this](const Message<0>& m) {
            events.push_back("Sync A " + m.data);

            // Sleep for some time to be safe
            NUClear::util::precise_sleep(test_util::TimeUnit(1));

            // Emit a message 1 here, it should not run yet
            events.push_back("Sync A emitting");
            emit(std::make_unique<Message<1>>("From Sync A"));

            // Sleep for some time again
            NUClear::util::precise_sleep(test_util::TimeUnit(1));

            events.push_back("Sync A " + m.data + " finished");
        });

        on<Trigger<Message<0>>, Sync<TestReactor>>().then([this](const Message<0>& m) {
            events.push_back("Sync B " + m.data);

            // Sleep for some time to be safe
            NUClear::util::precise_sleep(test_util::TimeUnit(1));

            // Emit a message 1 here, it should not run yet
            events.push_back("Sync B emitting");
            emit(std::make_unique<Message<1>>("From Sync B"));

            // Sleep for some time again
            NUClear::util::precise_sleep(test_util::TimeUnit(1));

            events.push_back("Sync B " + m.data + " finished");
        });

        on<Trigger<Message<1>>, Sync<TestReactor>>().then([this](const Message<1>& m) {
            events.push_back("Sync C " + m.data);

            // Sleep for some time to be safe
            NUClear::util::precise_sleep(test_util::TimeUnit(1));

            // Emit a message 1 here, it should not run yet
            events.push_back("Sync C waiting");

            // Sleep for some time again
            NUClear::util::precise_sleep(test_util::TimeUnit(1));

            events.push_back("Sync C " + m.data + " finished");

            if (m.data == "From Sync B") {
                powerplant.shutdown();
            }
        });

        on<Startup>().then([this] {  //
            emit(std::make_unique<Message<0>>("From Startup"));
        });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Testing that the Sync word works correctly", "[api][sync]") {
    NUClear::Configuration config;
    config.default_pool_concurrency = 4;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Sync A From Startup",
        "Sync A emitting",
        "Sync A From Startup finished",
        "Sync B From Startup",
        "Sync B emitting",
        "Sync B From Startup finished",
        "Sync C From Sync A",
        "Sync C waiting",
        "Sync C From Sync A finished",
        "Sync C From Sync B",
        "Sync C waiting",
        "Sync C From Sync B finished",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
