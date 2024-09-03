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
    struct MessageA {};
    struct MessageB {};

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        on<Trigger<MessageA>, With<MessageB>>().then([this](const MessageA&, const MessageB&) {  //
            events.push_back("Executed reaction with A and B");
        });

        on<Trigger<MessageA>, Optional<With<MessageB>>>().then([this](const std::shared_ptr<const MessageB>& b) {
            events.push_back(std::string("Executed reaction with A and optional B with B") + (b ? "+" : "-"));
            // Emit B to start the second set
            events.push_back("Emitting B");
            emit(std::make_unique<MessageB>());
        });

        on<Trigger<MessageB>, With<MessageA>>().then([this] {  //
            events.push_back("Executed reaction with B and A");
        });

        // Double trigger test (to ensure that it can handle multiple DSL words
        on<Optional<Trigger<MessageA>, Trigger<MessageB>>>().then(
            [this](const std::shared_ptr<const MessageA>& a, const std::shared_ptr<const MessageB>& b) {  //
                events.push_back(std::string("Executed reaction with optional A and B with A") + (a ? "+" : "-")
                                 + " and B" + (b ? "+" : "-"));
            });

        on<Startup>().then([this] {
            // Emit only message A
            events.push_back("Emitting A");
            emit(std::make_unique<MessageA>());
        });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Testing that optional is able to let data through even if it's invalid", "[api][optional]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Emitting A",
        "Executed reaction with A and optional B with B-",
        "Emitting B",
        "Executed reaction with optional A and B with A+ and B-",
        "Executed reaction with B and A",
        "Executed reaction with optional A and B with A+ and B+",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
