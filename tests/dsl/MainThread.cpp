/*
 * MIT License
 *
 * Copyright (c) 2016 NUClear Contributors
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

#include <catch.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

/// @brief Events that occur during the test
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

struct MessageA {};
struct MessageB {};

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        // Run a task without MainThread to make sure it isn't on the main thread
        on<Trigger<MessageA>>().then("Non-MainThread reaction", [this] {
            events.push_back(std::string("MessageA triggered ")
                             + (NUClear::util::main_thread_id == std::this_thread::get_id() ? "on main thread"
                                                                                            : "on non-main thread"));

            events.push_back("Emitting MessageB");
            emit(std::make_unique<MessageB>());
        });

        // Run a task with MainThread and ensure that it is on the main thread
        on<Trigger<MessageB>, MainThread>().then("MainThread reaction", [this] {
            events.push_back(std::string("MessageB triggered ")
                             + (NUClear::util::main_thread_id == std::this_thread::get_id() ? "on main thread"
                                                                                            : "on non-main thread"));

            // Since we are a multithreaded test with MainThread we need to shutdown the test ourselves
            powerplant.shutdown();
        });

        on<Startup>().then([this]() {
            // Emit an integer to trigger the reaction
            events.push_back("Emitting MessageA");
            emit(std::make_unique<MessageA>());
        });
    }
};
}  // namespace

TEST_CASE("Testing that the MainThread keyword runs tasks on the main thread", "[api][dsl][main_thread]") {
    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Emitting MessageA",
        "MessageA triggered on non-main thread",
        "Emitting MessageB",
        "MessageB triggered on main thread",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
