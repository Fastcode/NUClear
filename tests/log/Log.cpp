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

// This is a free floating function that we can use to test the log function when not in a reactor
template <NUClear::LogLevel level, typename... Args>
void free_floating_log(const Args&... args) {
    NUClear::log<level>(args...);
}

struct LogTestOutput {
    std::string message;
    NUClear::LogLevel level;
    bool from_reaction;
};

// Store all the log messages we received
std::vector<LogTestOutput> messages;

// All the log levels
const std::vector<NUClear::LogLevel> levels = {
    NUClear::TRACE, NUClear::DEBUG, NUClear::INFO, NUClear::WARN, NUClear::ERROR, NUClear::FATAL};

struct TestLevel {
    TestLevel(NUClear::LogLevel level) : level(level) {}
    NUClear::LogLevel level;
};

struct ShutdownOnIdle {};

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Capture the log messages
        on<Trigger<NUClear::message::LogMessage>>().then([this](const NUClear::message::LogMessage& log_message) {
            messages.push_back(LogTestOutput{log_message.message, log_message.level, log_message.task != nullptr});
        });

        // Run each test
        on<Trigger<TestLevel>>().then([this](const TestLevel& l) {
            // Limit the log level to the one we are testing
            this->log_level = l.level;

            // Test logs from a reaction
            log<NUClear::TRACE>("Direct Reaction", NUClear::TRACE);
            log<NUClear::DEBUG>("Direct Reaction", NUClear::DEBUG);
            log<NUClear::INFO>("Direct Reaction", NUClear::INFO);
            log<NUClear::WARN>("Direct Reaction", NUClear::WARN);
            log<NUClear::ERROR>("Direct Reaction", NUClear::ERROR);
            log<NUClear::FATAL>("Direct Reaction", NUClear::FATAL);

            // Test logs from a free floating function (called from a reaction)
            free_floating_log<NUClear::TRACE>("Indirect Reaction", NUClear::TRACE);
            free_floating_log<NUClear::DEBUG>("Indirect Reaction", NUClear::DEBUG);
            free_floating_log<NUClear::INFO>("Indirect Reaction", NUClear::INFO);
            free_floating_log<NUClear::WARN>("Indirect Reaction", NUClear::WARN);
            free_floating_log<NUClear::ERROR>("Indirect Reaction", NUClear::ERROR);
            free_floating_log<NUClear::FATAL>("Indirect Reaction", NUClear::FATAL);

            // Test logs called from a free floating function in another thread
            std::thread([this] {
                free_floating_log<NUClear::TRACE>("Non Reaction", NUClear::TRACE);
                free_floating_log<NUClear::DEBUG>("Non Reaction", NUClear::DEBUG);
                free_floating_log<NUClear::INFO>("Non Reaction", NUClear::INFO);
                free_floating_log<NUClear::WARN>("Non Reaction", NUClear::WARN);
                free_floating_log<NUClear::ERROR>("Non Reaction", NUClear::ERROR);
                free_floating_log<NUClear::FATAL>("Non Reaction", NUClear::FATAL);
            }).join();
        });

        // Shutdown when we have no tasks running
        on<Trigger<ShutdownOnIdle>, Priority::IDLE>().then([this] { powerplant.shutdown(); });

        on<Startup>().then([this] {
            // Test each log level
            for (const auto& level : levels) {
                emit(std::make_unique<TestLevel>(level));
            }

            emit(std::make_unique<ShutdownOnIdle>());
        });
    }
};
}  // namespace

TEST_CASE("Testing the Log<>() function", "[api][log]") {

    // Build with one thread
    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);

    // Install the test reactor
    plant.install<TestReactor>();
    plant.start();

    // Test that we have the correct number of messages
    REQUIRE(messages.size() == 78);


    // Test that each of the messages are correct for each log level
    int i = 0;
    for (const auto& display_level : levels) {
        // Test logs from reactions directly
        for (const auto& log_level : levels) {
            if (display_level <= log_level) {
                std::string expected = "Direct Reaction " + std::to_string(log_level);
                REQUIRE(messages[i].message == expected);
                REQUIRE(messages[i].level == log_level);
                REQUIRE(messages[i++].from_reaction);
            }
        }
        // Test logs from reactions indirectly
        for (const auto& log_level : levels) {
            if (display_level <= log_level) {
                std::string expected = "Indirect Reaction " + std::to_string(log_level);
                REQUIRE(messages[i].message == expected);
                REQUIRE(messages[i].level == log_level);
                REQUIRE(messages[i++].from_reaction);
            }
        }
        // Test logs from free floating functions
        for (const auto& log_level : levels) {
            // No filter here, free floating prints everything
            std::string expected = "Non Reaction " + std::to_string(log_level);
            REQUIRE(messages[i].message == expected);
            REQUIRE(messages[i].level == log_level);
            REQUIRE_FALSE(messages[i++].from_reaction);
        }
    }
}
