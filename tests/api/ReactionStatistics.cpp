/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

// Anonymous namespace to keep everything file local
namespace {

template <int id>
struct Message {};

struct LoopMsg {};

bool seen_message0        = false;
bool seen_message_startup = false;

using NUClear::message::ReactionStatistics;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Trigger<ReactionStatistics>>().then("Reaction Stats Handler 2", [this](const ReactionStatistics&) {
            // This reaction is just here to cause a potential looping of reaction statistics handlers
            emit(std::make_unique<LoopMsg>());
        });

        on<Trigger<LoopMsg>>().then("NoStats", [] {
            // This guy is triggered by someone triggering on reaction statistics, don't run
        });

        on<Trigger<ReactionStatistics>>().then("Reaction Stats Handler", [this](const ReactionStatistics& stats) {
            // If we are seeing ourself, fail
            REQUIRE(stats.identifier[0] != "Reaction Stats Handler");

            // If we are seeing the other reaction statistics handler, fail
            REQUIRE(stats.identifier[0] != "Reaction Stats Handler 2");

            // If we are seeing the other reaction statistics handler, fail
            REQUIRE(stats.identifier[0] != "NoStats");

            // Flag if we have seen the message handler
            if (stats.identifier[0] == "Message Handler") { seen_message0 = true; }
            // Flag if we have seen the startup handler
            else if (stats.identifier[0] == "Startup Handler") {
                seen_message_startup = true;
            }

            // Ensure exceptions are passed through correctly in the exception handler
            if (stats.exception) {
                REQUIRE(stats.identifier[0] == "Exception Handler");
                try {
                    std::rethrow_exception(stats.exception);
                }
                catch (const std::exception& e) {
                    REQUIRE(seen_message0);
                    REQUIRE(seen_message_startup);
                    REQUIRE(std::string(e.what()) == std::string("Exceptions happened"));

                    // We are done
                    powerplant.shutdown();
                }
            }
        });

        on<Trigger<Message<0>>>().then("Message Handler", [this] { emit(std::make_unique<Message<1>>()); });

        on<Trigger<Message<1>>>().then("Exception Handler", [] { throw std::runtime_error("Exceptions happened"); });

        on<Startup>().then("Startup Handler", [this] { emit(std::make_unique<Message<0>>()); });
    }
};
}  // namespace

TEST_CASE("Testing reaction statistics functionality", "[api][reactionstatistics]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);

    // We are installing with an initial log level of debug
    plant.install<TestReactor>();

    plant.start();
}
