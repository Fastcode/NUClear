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
#include <iostream>
#include <nuclear>

#include "test_util/common.hpp"
#include "test_util/executable_path.hpp"

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

/// All the log messages received
std::vector<LogTestOutput> messages;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

// All the log levels
// NOLINTNEXTLINE(cert-err58-cpp,cppcoreguidelines-avoid-non-const-global-variables)
const std::vector<NUClear::LogLevel> levels =
    {NUClear::TRACE, NUClear::DEBUG, NUClear::INFO, NUClear::WARN, NUClear::ERROR, NUClear::FATAL};

struct TestLevel {
    TestLevel(NUClear::LogLevel level) : level(level) {}
    NUClear::LogLevel level;
};

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Capture the log messages
        on<Trigger<NUClear::message::LogMessage>>().then([](const NUClear::message::LogMessage& log_message) {
            if (log_message.level >= log_message.display_level) {
                messages.push_back(
                    LogTestOutput{log_message.message, log_message.level, log_message.statistics != nullptr});
            }
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
            std::thread([] {
                free_floating_log<NUClear::TRACE>("Non Reaction", NUClear::TRACE);
                free_floating_log<NUClear::DEBUG>("Non Reaction", NUClear::DEBUG);
                free_floating_log<NUClear::INFO>("Non Reaction", NUClear::INFO);
                free_floating_log<NUClear::WARN>("Non Reaction", NUClear::WARN);
                free_floating_log<NUClear::ERROR>("Non Reaction", NUClear::ERROR);
                free_floating_log<NUClear::FATAL>("Non Reaction", NUClear::FATAL);
            }).join();
        });

        // Shutdown when we have no tasks running
        on<Idle<>>().then([this] {
            powerplant.shutdown();

            free_floating_log<NUClear::TRACE>("Post Powerplant Shutdown", NUClear::TRACE);
            free_floating_log<NUClear::DEBUG>("Post Powerplant Shutdown", NUClear::DEBUG);
            free_floating_log<NUClear::INFO>("Post Powerplant Shutdown", NUClear::INFO);
            free_floating_log<NUClear::WARN>("Post Powerplant Shutdown", NUClear::WARN);
            free_floating_log<NUClear::ERROR>("Post Powerplant Shutdown", NUClear::ERROR);
            free_floating_log<NUClear::FATAL>("Post Powerplant Shutdown", NUClear::FATAL);

            log<NUClear::TRACE>("Post Powerplant Shutdown", NUClear::TRACE);
            log<NUClear::DEBUG>("Post Powerplant Shutdown", NUClear::DEBUG);
            log<NUClear::INFO>("Post Powerplant Shutdown", NUClear::INFO);
            log<NUClear::WARN>("Post Powerplant Shutdown", NUClear::WARN);
            log<NUClear::ERROR>("Post Powerplant Shutdown", NUClear::ERROR);
            log<NUClear::FATAL>("Post Powerplant Shutdown", NUClear::FATAL);

            std::thread([] {
                free_floating_log<NUClear::TRACE>("Non Reaction", NUClear::TRACE);
                free_floating_log<NUClear::DEBUG>("Non Reaction", NUClear::DEBUG);
                free_floating_log<NUClear::INFO>("Non Reaction", NUClear::INFO);
                free_floating_log<NUClear::WARN>("Non Reaction", NUClear::WARN);
                free_floating_log<NUClear::ERROR>("Non Reaction", NUClear::ERROR);
                free_floating_log<NUClear::FATAL>("Non Reaction", NUClear::FATAL);
            }).join();
        });

        on<Startup>().then([this] {
            // Test each log level
            for (const auto& level : levels) {
                emit(std::make_unique<TestLevel>(level));
            }
        });
    }
};


TEST_CASE("Testing the Log<>() function", "[api][log]") {

    // Try to call log before constructing a powerplant
    free_floating_log<NUClear::TRACE>("Pre Powerplant Construction", NUClear::TRACE);
    free_floating_log<NUClear::DEBUG>("Pre Powerplant Construction", NUClear::DEBUG);
    free_floating_log<NUClear::INFO>("Pre Powerplant Construction", NUClear::INFO);
    free_floating_log<NUClear::WARN>("Pre Powerplant Construction", NUClear::WARN);
    free_floating_log<NUClear::ERROR>("Pre Powerplant Construction", NUClear::ERROR);
    free_floating_log<NUClear::FATAL>("Pre Powerplant Construction", NUClear::FATAL);

    // Local scope to force powerplant destruction
    {
        // Build with one thread
        NUClear::Configuration config;
        config.default_pool_concurrency = 1;
        NUClear::PowerPlant plant(config);

        // Install the test reactor
        test_util::add_tracing(plant);
        plant.install<TestReactor>();
        plant.start();
    }

    // Try to call log after destructing the powerplant
    free_floating_log<NUClear::TRACE>("Post Powerplant Destruction", NUClear::TRACE);
    free_floating_log<NUClear::DEBUG>("Post Powerplant Destruction", NUClear::DEBUG);
    free_floating_log<NUClear::INFO>("Post Powerplant Destruction", NUClear::INFO);
    free_floating_log<NUClear::WARN>("Post Powerplant Destruction", NUClear::WARN);
    free_floating_log<NUClear::ERROR>("Post Powerplant Destruction", NUClear::ERROR);
    free_floating_log<NUClear::FATAL>("Post Powerplant Destruction", NUClear::FATAL);

    // Test that we have the correct number of messages
    size_t expected_count = 0;
    expected_count += levels.size() * (levels.size() + 1) / 2;  // Direct reaction logs
    expected_count += levels.size() * (levels.size() + 1) / 2;  // Indirect reaction logs
    expected_count += levels.size() * levels.size();            // Non reaction logs
    expected_count += 2 + levels.size();                        // Post shutdown logs
    REQUIRE(messages.size() == expected_count);

    // Test that each of the messages are correct for each log level
    int i = 0;
    for (const auto& display_level : levels) {
        // Test logs from reactions directly
        for (const auto& log_level : levels) {
            if (display_level <= log_level) {
                const std::string expected = "Direct Reaction " + NUClear::to_string(log_level);
                REQUIRE(messages[i].message == expected);
                REQUIRE(messages[i].level == log_level);
                REQUIRE(messages[i++].from_reaction);
            }
        }
        // Test logs from reactions indirectly
        for (const auto& log_level : levels) {
            if (display_level <= log_level) {
                const std::string expected = "Indirect Reaction " + NUClear::to_string(log_level);
                REQUIRE(messages[i].message == expected);
                REQUIRE(messages[i].level == log_level);
                REQUIRE(messages[i++].from_reaction);
            }
        }
        // Test logs from free floating functions
        for (const auto& log_level : levels) {
            // No filter here, free floating prints everything
            const std::string expected = "Non Reaction " + NUClear::to_string(log_level);
            REQUIRE(messages[i].message == expected);
            REQUIRE(messages[i].level == log_level);
            REQUIRE_FALSE(messages[i++].from_reaction);
        }
    }

    // Test post-shutdown logs
    {
        const std::string expected = "Post Powerplant Shutdown " + NUClear::to_string(NUClear::FATAL);
        REQUIRE(messages[i].message == expected);
        REQUIRE(messages[i].level == NUClear::FATAL);
        REQUIRE(messages[i++].from_reaction);
        REQUIRE(messages[i].message == expected);
        REQUIRE(messages[i].level == NUClear::FATAL);
        REQUIRE(messages[i++].from_reaction);
    }

    // Test logs from free floating functions
    for (const auto& log_level : levels) {
        // No filter here, free floating prints everything
        const std::string expected = "Non Reaction " + NUClear::to_string(log_level);
        REQUIRE(messages[i].message == expected);
        REQUIRE(messages[i].level == log_level);
        REQUIRE_FALSE(messages[i++].from_reaction);
    }
}
