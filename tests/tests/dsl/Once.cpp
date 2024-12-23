/*
 * MIT License
 *
 * Copyright (c) 2021 NUClear Contributors
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

class TestReactor : public NUClear::Reactor {
public:
    struct SimpleMessage {
        SimpleMessage(int run) : run(run) {}
        int run = 0;
    };

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Make this priority high so it will always run first if it is able
        on<Trigger<SimpleMessage>, Priority::HIGH, Once>().then([this](const SimpleMessage& msg) {  //
            events.push_back("Once Trigger executed " + std::to_string(msg.run));
        });

        on<Trigger<SimpleMessage>>().then([this](const SimpleMessage& msg) {
            events.push_back("Normal Trigger Executed " + std::to_string(msg.run));
            // Keep running until we have run 10 times
            if (msg.run < 10) {
                events.push_back("Emitting " + std::to_string(msg.run + 1));
                emit(std::make_unique<SimpleMessage>(msg.run + 1));
            }
            else {
                powerplant.shutdown();
            }
        });

        on<Startup>().then([this] {
            events.push_back("Startup Trigger Executed");
            emit(std::make_unique<SimpleMessage>(0));
        });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Reactions with the Once DSL keyword only execute once", "[api][once]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Startup Trigger Executed",   "Once Trigger executed 0",
        "Normal Trigger Executed 0",  "Emitting 1",
        "Normal Trigger Executed 1",  "Emitting 2",
        "Normal Trigger Executed 2",  "Emitting 3",
        "Normal Trigger Executed 3",  "Emitting 4",
        "Normal Trigger Executed 4",  "Emitting 5",
        "Normal Trigger Executed 5",  "Emitting 6",
        "Normal Trigger Executed 6",  "Emitting 7",
        "Normal Trigger Executed 7",  "Emitting 8",
        "Normal Trigger Executed 8",  "Emitting 9",
        "Normal Trigger Executed 9",  "Emitting 10",
        "Normal Trigger Executed 10",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
