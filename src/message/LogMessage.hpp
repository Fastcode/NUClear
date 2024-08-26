/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#ifndef NUCLEAR_MESSAGE_LOG_MESSAGE_HPP
#define NUCLEAR_MESSAGE_LOG_MESSAGE_HPP

#include <memory>

#include "../LogLevel.hpp"
#include "ReactionStatistics.hpp"

namespace NUClear {
namespace message {

    /**
     * This type is a NUClear message type that holds a log message.
     */
    struct LogMessage {

        /**
         * Construct a new Log Message object.
         *
         * @param level         The logging level of the log
         * @param display_level The logging level of the reactor that made this log
         * @param message       The string contents of the message
         * @param statistics    The statistics of the currently executing task or nullptr if not in a task
         */
        LogMessage(const LogLevel& level,
                   const LogLevel& display_level,
                   std::string message,
                   std::shared_ptr<const ReactionStatistics> statistics)
            : level(level)
            , display_level(display_level)
            , message(std::move(message))
            , statistics(std::move(statistics)) {}

        /// The logging level of the log
        LogLevel level{};

        /// The logging level of the reactor that made the log (the level to display at)
        LogLevel display_level{};

        /// The string contents of the message
        std::string message;

        /// The statistics of the currently executing task that made this message
        const std::shared_ptr<const ReactionStatistics> statistics{nullptr};
    };

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_LOG_MESSAGE_HPP
