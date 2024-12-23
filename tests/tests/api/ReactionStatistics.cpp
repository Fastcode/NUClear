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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

// This namespace is named to make things consistent with the reaction statistics test

using NUClear::message::ReactionEvent;

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    template <int id>
    struct Message {};
    struct LoopMessage {};

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        // This reaction is here to emit something from a ReactionStatistics trigger
        // This shouldn't cause reaction statistics of their own otherwise everything would explode
        on<Trigger<ReactionEvent>, Priority::HIGH>().then("Loop Statistics", [this](const ReactionEvent&) {  //
            emit(std::make_unique<LoopMessage>());
        });
        on<Trigger<LoopMessage>>().then("No Statistics", [] {});


        on<Trigger<ReactionEvent>, Priority::HIGH>().then("Reaction Stats Handler", [this](const ReactionEvent& event) {
            const auto& stats = *event.statistics;

            // Other reactions statistics run on this because of built in NUClear reactors (e.g. chrono controller etc)
            // We want to filter those out so only our own stats are shown
            if (stats.identifiers->name.empty() || stats.identifiers->reactor != reactor_name) {
                return;
            }

            // Skip stats until the finished event
            if (event.type != ReactionEvent::FINISHED) {
                return;
            }

            events.push_back("Stats for " + stats.identifiers->name + " from " + stats.identifiers->reactor);
            events.push_back(stats.identifiers->dsl);

            // Ensure exceptions are passed through correctly in the exception handler
            if (stats.exception) {
                try {
                    std::rethrow_exception(stats.exception);
                }
                catch (const std::exception& e) {
                    events.push_back("Exception received: \"" + std::string(e.what()) + "\"");
                }
            }
        });

        on<Trigger<Message<1>>>().then("Exception Handler", [this] {
            events.push_back("Running Exception Handler");
            throw std::runtime_error("Text in an exception");
        });

        on<Trigger<Message<0>>>().then("Message Handler", [this] {
            events.push_back("Running Message Handler");
            emit(std::make_unique<Message<1>>());
        });

        on<Startup>().then("Startup Handler", [this] {
            events.push_back("Running Startup Handler");
            emit(std::make_unique<Message<0>>());
        });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Testing reaction statistics functionality", "[api][reactionstatistics]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Running Startup Handler",
        "Stats for Startup Handler from TestReactor",
        "NUClear::Reactor::on<NUClear::dsl::word::Startup>",
        "Running Message Handler",
        "Stats for Message Handler from TestReactor",
        "NUClear::Reactor::on<NUClear::dsl::word::Trigger<TestReactor::Message<0>>>",
        "Running Exception Handler",
        "Stats for Exception Handler from TestReactor",
        "NUClear::Reactor::on<NUClear::dsl::word::Trigger<TestReactor::Message<1>>>",
        "Exception received: \"Text in an exception\"",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
