/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_MESSAGE_LOGMESSAGE_HPP
#define NUCLEAR_MESSAGE_LOGMESSAGE_HPP

#include "nuclear_bits/LogLevel.hpp"
#include "nuclear_bits/message/ReactionStatistics.hpp"

namespace NUClear {
namespace message {

    /**
     * @brief This type is a NUClear message type that holds a log message.
     */
    struct LogMessage {


        /// @brief The logging level of the log.
        LogLevel level;

        /// @brief The string contents of the message.
        std::string message;

        /// @brief The currently executing task that made this message
        const ReactionStatistics* task;
    };

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_LOGMESSAGE_HPP
