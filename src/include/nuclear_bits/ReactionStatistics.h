/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_MESSAGES_REACTIONSTATISTICS_H
#define NUCLEAR_MESSAGES_REACTIONSTATISTICS_H

#include <string>

#include "nuclear_bits/clock.h"

namespace NUClear {
    
    /**
     * @brief Holds details about reactions that are executed.
     *
     * @author Trent Houliston
     */
    struct ReactionStatistics {
        /// @brief A string containing the username/on arguments/and callback name of the reaction.
        std::string name;
        /// @brief The id of this reaction.
        std::uint64_t reactionId;
        /// @brief The task id of this reaction.
        std::uint64_t taskId;
        /// @brief The reaction id of the reaction that caused this one or 0 if there was not one
        std::uint64_t causeReactionId;
        /// @brief The reaction id of the task that caused this task or 0 if there was not one
        std::uint64_t causeTaskId;
        /// @brief The time that this reaction was emitted to the thread pool
        clock::time_point emitted;
        /// @brief The time that execution started on this reaction
        clock::time_point started;
        /// @brief The time that execution finished on this reaction
        clock::time_point finished;
        /// @brief An exception pointer that can be rethrown (if the reaction threw an exception)
        std::exception_ptr exception;
    };
}
#endif
