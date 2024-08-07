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

namespace {

/// @brief Events that occur during the test
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

struct SimpleMessage {};

struct MessageA {};
struct MessageB {};

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {


        on<Trigger<MessageA>>().then([this] {
            events.push_back("MessageA triggered");
            events.push_back("Emitting MessageB");
            emit(std::make_unique<MessageB>());
        });

        on<Trigger<MessageB>>().then([] {  //
            events.push_back("MessageB triggered");
        });

        // This should never run
        on<Trigger<MessageA>, With<MessageB>>().then([](const MessageA&, const MessageB&) {  //
            events.push_back("MessageA with MessageB triggered");
        });

        on<Trigger<Step<1>>, Priority::LOW>().then([this] {
            events.push_back("Step<1> triggered");
            events.push_back("Emitting MessageA");
            emit(std::make_unique<MessageA>());
        });

        on<Startup>().then([this] {
            events.push_back("Emitting Step<1>");
            emit(std::make_unique<Step<1>>());
        });
    }
};
}  // namespace


TEST_CASE("Testing emitting types that are flag types (Have no contents)", "[api][flag]") {

    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Emitting Step<1>",
        "Step<1> triggered",
        "Emitting MessageA",
        "MessageA triggered",
        "Emitting MessageB",
        "MessageB triggered",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
