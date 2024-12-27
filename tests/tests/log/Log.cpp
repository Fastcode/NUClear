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
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nuclear"
#include "test_util/common.hpp"

namespace {  // Anonymous namespace for internal linkage

// This is a free floating function that we can use to test the log function when not in a reactor
template <NUClear::LogLevel::Value level, typename... Args>
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

}  // namespace

// All the log levels
// NOLINTNEXTLINE(cert-err58-cpp,cppcoreguidelines-avoid-non-const-global-variables)
const std::vector<NUClear::LogLevel> levels = {NUClear::LogLevel::TRACE,
                                               NUClear::LogLevel::DEBUG,
                                               NUClear::LogLevel::INFO,
                                               NUClear::LogLevel::WARN,
                                               NUClear::LogLevel::ERROR,
                                               NUClear::LogLevel::FATAL};

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
            log<TRACE>("Direct Reaction", TRACE);
            log<DEBUG>("Direct Reaction", DEBUG);
            log<INFO>("Direct Reaction", INFO);
            log<WARN>("Direct Reaction", WARN);
            log<ERROR>("Direct Reaction", ERROR);
            log<FATAL>("Direct Reaction", FATAL);

            // Test logs from a free floating function (called from a reaction)
            free_floating_log<TRACE>("Indirect Reaction", TRACE);
            free_floating_log<DEBUG>("Indirect Reaction", DEBUG);
            free_floating_log<INFO>("Indirect Reaction", INFO);
            free_floating_log<WARN>("Indirect Reaction", WARN);
            free_floating_log<ERROR>("Indirect Reaction", ERROR);
            free_floating_log<FATAL>("Indirect Reaction", FATAL);

            // Test logs called from a free floating function in another thread
            std::thread([] {
                free_floating_log<TRACE>("Non Reaction", TRACE);
                free_floating_log<DEBUG>("Non Reaction", DEBUG);
                free_floating_log<INFO>("Non Reaction", INFO);
                free_floating_log<WARN>("Non Reaction", WARN);
                free_floating_log<ERROR>("Non Reaction", ERROR);
                free_floating_log<FATAL>("Non Reaction", FATAL);
            }).join();
        });

        // Shutdown when we have no tasks running
        on<Idle<>>().then([this] {
            powerplant.shutdown();

            free_floating_log<TRACE>("Post Powerplant Shutdown", TRACE);
            free_floating_log<DEBUG>("Post Powerplant Shutdown", DEBUG);
            free_floating_log<INFO>("Post Powerplant Shutdown", INFO);
            free_floating_log<WARN>("Post Powerplant Shutdown", WARN);
            free_floating_log<ERROR>("Post Powerplant Shutdown", ERROR);
            free_floating_log<FATAL>("Post Powerplant Shutdown", FATAL);

            log<TRACE>("Post Powerplant Shutdown", TRACE);
            log<DEBUG>("Post Powerplant Shutdown", DEBUG);
            log<INFO>("Post Powerplant Shutdown", INFO);
            log<WARN>("Post Powerplant Shutdown", WARN);
            log<ERROR>("Post Powerplant Shutdown", ERROR);
            log<FATAL>("Post Powerplant Shutdown", FATAL);

            std::thread([] {
                free_floating_log<TRACE>("Non Reaction", TRACE);
                free_floating_log<DEBUG>("Non Reaction", DEBUG);
                free_floating_log<INFO>("Non Reaction", INFO);
                free_floating_log<WARN>("Non Reaction", WARN);
                free_floating_log<ERROR>("Non Reaction", ERROR);
                free_floating_log<FATAL>("Non Reaction", FATAL);
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

    using LogLevel = NUClear::LogLevel;

    // Try to call log before constructing a powerplant
    free_floating_log<NUClear::LogLevel::TRACE>("Pre Powerplant Construction", LogLevel(LogLevel::TRACE));
    free_floating_log<NUClear::LogLevel::DEBUG>("Pre Powerplant Construction", LogLevel(LogLevel::DEBUG));
    free_floating_log<NUClear::LogLevel::INFO>("Pre Powerplant Construction", LogLevel(LogLevel::INFO));
    free_floating_log<NUClear::LogLevel::WARN>("Pre Powerplant Construction", LogLevel(LogLevel::WARN));
    free_floating_log<NUClear::LogLevel::ERROR>("Pre Powerplant Construction", LogLevel(LogLevel::ERROR));
    free_floating_log<NUClear::LogLevel::FATAL>("Pre Powerplant Construction", LogLevel(LogLevel::FATAL));

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
    free_floating_log<NUClear::LogLevel::TRACE>("Post Powerplant Destruction", LogLevel(LogLevel::TRACE));
    free_floating_log<NUClear::LogLevel::DEBUG>("Post Powerplant Destruction", LogLevel(LogLevel::DEBUG));
    free_floating_log<NUClear::LogLevel::INFO>("Post Powerplant Destruction", LogLevel(LogLevel::INFO));
    free_floating_log<NUClear::LogLevel::WARN>("Post Powerplant Destruction", LogLevel(LogLevel::WARN));
    free_floating_log<NUClear::LogLevel::ERROR>("Post Powerplant Destruction", LogLevel(LogLevel::ERROR));
    free_floating_log<NUClear::LogLevel::FATAL>("Post Powerplant Destruction", LogLevel(LogLevel::FATAL));

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
                const std::string expected = "Direct Reaction " + static_cast<std::string>(log_level);
                REQUIRE(messages[i].message == expected);
                REQUIRE(messages[i].level == log_level);
                REQUIRE(messages[i++].from_reaction);
            }
        }
        // Test logs from reactions indirectly
        for (const auto& log_level : levels) {
            if (display_level <= log_level) {
                const std::string expected = "Indirect Reaction " + static_cast<std::string>(log_level);
                REQUIRE(messages[i].message == expected);
                REQUIRE(messages[i].level == log_level);
                REQUIRE(messages[i++].from_reaction);
            }
        }
        // Test logs from free floating functions
        for (const auto& log_level : levels) {
            // No filter here, free floating prints everything
            const std::string expected = "Non Reaction " + static_cast<std::string>(log_level);
            REQUIRE(messages[i].message == expected);
            REQUIRE(messages[i].level == log_level);
            REQUIRE_FALSE(messages[i++].from_reaction);
        }
    }

    // Test post-shutdown logs
    {
        const std::string expected = "Post Powerplant Shutdown " + static_cast<std::string>(LogLevel(LogLevel::FATAL));
        REQUIRE(messages[i].message == expected);
        REQUIRE(messages[i].level == LogLevel::FATAL);
        REQUIRE(messages[i++].from_reaction);
        REQUIRE(messages[i].message == expected);
        REQUIRE(messages[i].level == LogLevel::FATAL);
        REQUIRE(messages[i++].from_reaction);
    }

    // Test logs from free floating functions
    for (const auto& log_level : levels) {
        // No filter here, free floating prints everything
        const std::string expected = "Non Reaction " + static_cast<std::string>(log_level);
        REQUIRE(messages[i].message == expected);
        REQUIRE(messages[i].level == log_level);
        REQUIRE_FALSE(messages[i++].from_reaction);
    }
}
