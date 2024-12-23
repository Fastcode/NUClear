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

#ifndef NUCLEAR_LOGLEVEL_HPP
#define NUCLEAR_LOGLEVEL_HPP

#include <ostream>

// Why do we need to include platform.hpp here?
// Because windows defines a bunch of things for legacy reasons, one of which is a #define for ERROR as blank
// Of course this causes a problem when we define our own token below as error as the preprocessor removes it
// The platform.hpp fixes this (and many other windows nonsense) so include it here to solve that issue
#include "util/platform.hpp"

namespace NUClear {

/**
 * LogLevel defines the different levels log messages can be set to.
 *
 * Log levels are used to provide different levels of detail on a per-reactor basis.
 * The logging level of a reactor can be changed by setting it in the install function.
 */
class LogLevel {
public:
    enum Value : uint8_t {
        /**
         * Don't use this log level when emitting logs, it is for setting reactor log level from non reactor sources.
         *
         * Specifically when a NUClear::log is called from code that is not running in a reaction (even transitively)
         * then the reactor_level will be set to UNKNOWN.
         */
        UNKNOWN,

        /**
         * The Trace level contains messages that are used to trace the exact flow of execution.
         *
         * This level is extremely verbose and often has a message per line of code.
         */
        TRACE,

        /**
         * Debug contains messages that represent the inputs and outputs of different computation units.
         *
         * If you have a function that performs three steps to do something then it's likely that you will have a
         * message for the input and output of those three steps. Additionally you would likely have messages that check
         * if it hit different branches.
         */
        DEBUG,

        /**
         * The info level is used to provide high level goal messages such as function start or successful completion.
         *
         * This shows when key user-facing functionality is executed and tells us that everything is working without
         * getting into the details.
         */
        INFO,

        /**
         * The warning level is used to notify us that everything might not be working perfectly.
         *
         * Warnings are errors or inconsistencies that aren't fatal and generally do not completely break the system.
         * However a warning message should require action and should point to a section of the system that needs
         * attention.
         */
        WARN,

        /**
         * The error level is used to report unexpected behavior.

         * This level doesn't need to prefix a program-crashing issue but should be used to report major unexpected
         branches
         * in logic or other constraint breaking problems such as failed assertions.
         * All errors should require action from someone and should be addressed immediately.
         */
        ERROR,

        /**
         * Fatal is a program destroying error that needs to be addressed immediately.
         *
         * If a fatal message is sent it should point to something that should never ever happen and ideally provide as
         * much information as possible as to why it crashed. Fatal messages require action immediately and should
         * always be addressed.
         */
        FATAL
    };

    /**
     * Construct a LogLevel from a Value
     *
     * @param value The value to construct the LogLevel from
     */
    constexpr LogLevel(const Value& value = Value::UNKNOWN) : value(value) {};

    /**
     * Construct a LogLevel from a string
     *
     * @param level The string to construct the LogLevel from
     */
    constexpr LogLevel(const std::string& level)
        : value(level == "TRACE"   ? LogLevel::TRACE
                : level == "DEBUG" ? LogLevel::DEBUG
                : level == "INFO"  ? LogLevel::INFO
                : level == "WARN"  ? LogLevel::WARN
                : level == "ERROR" ? LogLevel::ERROR
                : level == "FATAL" ? LogLevel::FATAL
                                   : LogLevel::UNKNOWN) {};

    /**
     * A call operator which will return the value of the LogLevel
     * This can be useful in situations where the implicit conversion operators are ambiguous.
     *
     * @return The value of the LogLevel
     */
    constexpr Value operator()() const {
        return value;
    }

    /**
     * A conversion operator which will return the value of the LogLevel
     *
     * @return The value of the LogLevel
     */
    constexpr operator Value() const {
        return value;
    }

    /**
     * A conversion operator which will return the string representation of the LogLevel
     *
     * @return The string representation of the LogLevel
     */
    operator std::string() const {
        return value == LogLevel::TRACE   ? "TRACE"
               : value == LogLevel::DEBUG ? "DEBUG"
               : value == LogLevel::INFO  ? "INFO"
               : value == LogLevel::WARN  ? "WARN"
               : value == LogLevel::ERROR ? "ERROR"
               : value == LogLevel::FATAL ? "FATAL"
                                          : "UNKNOWN";
    }

    /**
     * Stream the LogLevel to an ostream, it will output the string representation of the LogLevel
     *
     * @param os The ostream to output to
     * @param level The LogLevel to output
     *
     * @return The ostream that was passed in
     */
    friend std::ostream& operator<<(std::ostream& os, LogLevel level) {
        return os << static_cast<std::string>(level);
    }

    // Operators to compare LogLevel values and LogLevel to Value
    // clang-format off
    friend constexpr bool operator<(const LogLevel& lhs, const LogLevel& rhs) { return lhs.value < rhs.value; }
    friend constexpr bool operator>(const LogLevel& lhs, const LogLevel& rhs) { return lhs.value > rhs.value; }
    friend constexpr bool operator<=(const LogLevel& lhs, const LogLevel& rhs) { return lhs.value <= rhs.value; }
    friend constexpr bool operator>=(const LogLevel& lhs, const LogLevel& rhs) { return lhs.value >= rhs.value; }
    friend constexpr bool operator==(const LogLevel& lhs, const LogLevel& rhs) { return lhs.value == rhs.value; }
    friend constexpr bool operator!=(const LogLevel& lhs, const LogLevel& rhs) { return lhs.value != rhs.value; }
    friend constexpr bool operator<(const LogLevel& lhs, const Value& rhs) { return lhs.value < rhs; }
    friend constexpr bool operator>(const LogLevel& lhs, const Value& rhs) { return lhs.value > rhs; }
    friend constexpr bool operator<=(const LogLevel& lhs, const Value& rhs) { return lhs.value <= rhs; }
    friend constexpr bool operator>=(const LogLevel& lhs, const Value& rhs) { return lhs.value >= rhs; }
    friend constexpr bool operator==(const LogLevel& lhs, const Value& rhs) { return lhs.value == rhs; }
    friend constexpr bool operator!=(const LogLevel& lhs, const Value& rhs) { return lhs.value != rhs; }
    friend constexpr bool operator<(const Value& lhs, const LogLevel& rhs) { return lhs < rhs.value; }
    friend constexpr bool operator>(const Value& lhs, const LogLevel& rhs) { return lhs > rhs.value; }
    friend constexpr bool operator<=(const Value& lhs, const LogLevel& rhs) { return lhs <= rhs.value; }
    friend constexpr bool operator>=(const Value& lhs, const LogLevel& rhs) { return lhs >= rhs.value; }
    friend constexpr bool operator==(const Value& lhs, const LogLevel& rhs) { return lhs == rhs.value; }
    friend constexpr bool operator!=(const Value& lhs, const LogLevel& rhs) { return lhs != rhs.value; }
    // clang-format on


private:
    /// The stored enum value
    Value value;
};
}  // namespace NUClear

#endif  // NUCLEAR_LOGLEVEL_HPP
