/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_LOGGER_HPP
#define NUCLEAR_UTIL_LOGGER_HPP

#include <sstream>
#include <utility>

#include "../LogLevel.hpp"
#include "string_join.hpp"

namespace NUClear {

// Forward declarations
class PowerPlant;
class Reactor;

namespace util {

    /**
     * The Logger class is used to handle converting the variardic log calls into an appropriate log message.
     *
     * It's also responsible for working out which messages should be emitted and which should be ignored.
     */
    class Logger {
    public:
        Logger(PowerPlant& powerplant) : powerplant(powerplant) {}

        /**
         * Log a message to the system.
         *
         * This function will check if a log message should be emitted based on the current log levels and then emit the
         * log message.
         */
        template <typename... Arguments>
        void log(const Reactor* reactor, const LogLevel& level, const Arguments&... args) {
            // Check if the log level is above the minimum log levels
            auto log_levels = get_current_log_levels(reactor);
            if (level >= log_levels.display_log_level || level >= log_levels.min_log_level) {
                // Collapse the arguments into a string and perform the log action
                do_log(reactor, level, string_join(" ", args...));
            }
        }

    private:
        /**
         * Describes the log levels for a particular reactor.
         */
        struct LogLevels {
            LogLevels(const LogLevel& display_log_level, const LogLevel& min_log_level)
                : display_log_level(display_log_level), min_log_level(min_log_level) {}

            /// The log level that should be displayed
            LogLevel display_log_level;
            /// The minimum log level that should be emitted
            LogLevel min_log_level;
        };

        /**
         * Get the log levels for the current reactor or UNKNOWN if not in a reactor.
         */
        static LogLevels get_current_log_levels(const Reactor* reactor);

        /**
         * Perform the log action and emit the log message.
         */
        void do_log(const Reactor* reactor, const LogLevel& level, const std::string& message);

        /// The powerplant that this logger is logging for, used for emitting log messages
        PowerPlant& powerplant;
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_LOGGER_HPP
