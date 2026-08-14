/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
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
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "nuclearnet/Log.hpp"
#include "nuclearnet/NUClearNet.hpp"

namespace {

using NUClear::network::LogLevel;

/// A message as it was handed to the log handler
struct Message {
    LogLevel level;
    std::string component;
    std::string text;
};

/// Restore the default log state when the test finishes, however it finishes
struct LogGuard {
    LogGuard() = default;
    ~LogGuard() {
        NUClear::network::set_log_handler(nullptr);
        NUClear::network::set_log_level(LogLevel::Off);
    }
    LogGuard(const LogGuard&)            = delete;
    LogGuard(LogGuard&&)                 = delete;
    LogGuard& operator=(const LogGuard&) = delete;
    LogGuard& operator=(LogGuard&&)      = delete;
};

}  // namespace

SCENARIO("A log handler receives the messages that pass the log level", "[nuclearnet][log]") {
    const LogGuard guard;
    std::vector<Message> messages;

    GIVEN("a handler installed at the info level") {
        NUClear::network::set_log_level(LogLevel::Info);
        NUClear::network::set_log_handler(
            [&messages](LogLevel level, const char* component, const std::string& message) {
                messages.push_back(Message{level, component, message});
            });

        WHEN("messages are logged above, at and below that level") {
            NUClear::network::log(LogLevel::Error, "test", "an error");
            NUClear::network::log(LogLevel::Info, "test", "some info");
            NUClear::network::log(LogLevel::Debug, "test", "some debug");

            THEN("only the messages at or above the level reach the handler") {
                REQUIRE(messages.size() == 2);
                REQUIRE(messages[0].level == LogLevel::Error);
                REQUIRE(messages[0].component == "test");
                REQUIRE(messages[0].text == "an error");
                REQUIRE(messages[1].level == LogLevel::Info);
                REQUIRE(messages[1].text == "some info");
            }
        }
    }
}

SCENARIO("The log handler can be set through the NUClearNet class", "[nuclearnet][log]") {
    const LogGuard guard;
    std::vector<Message> messages;

    GIVEN("a handler installed through NUClearNet") {
        NUClear::network::NUClearNet::set_log_level(LogLevel::Trace);
        NUClear::network::NUClearNet::set_log_handler(
            [&messages](LogLevel level, const char* component, const std::string& message) {
                messages.push_back(Message{level, component, message});
            });

        WHEN("a message is logged") {
            NUClear::network::log(LogLevel::Trace, "binding", "hello");

            THEN("the handler receives it") {
                REQUIRE(messages.size() == 1);
                REQUIRE(messages[0].component == "binding");
                REQUIRE(messages[0].text == "hello");
            }
        }
    }
}

SCENARIO("Clearing the log handler restores the stderr sink", "[nuclearnet][log]") {
    const LogGuard guard;
    std::vector<Message> messages;

    GIVEN("a handler that is then cleared") {
        NUClear::network::set_log_level(LogLevel::Warn);
        NUClear::network::set_log_handler(
            [&messages](LogLevel level, const char* component, const std::string& message) {
                messages.push_back(Message{level, component, message});
            });
        NUClear::network::set_log_handler(nullptr);

        WHEN("a message is logged with stderr captured") {
            std::ostringstream captured;
            auto* const original = std::cerr.rdbuf(captured.rdbuf());
            NUClear::network::log(LogLevel::Warn, "test", "back to stderr");
            std::cerr.rdbuf(original);

            THEN("the handler is not called and the message is written to stderr") {
                REQUIRE(messages.empty());
                REQUIRE(captured.str() == "[NUClearNet:test] warn back to stderr\n");
            }
        }
    }
}
