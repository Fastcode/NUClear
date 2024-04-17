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

#ifndef NUCLEAR_MESSAGE_REACTIONSTATISTICS_HPP
#define NUCLEAR_MESSAGE_REACTIONSTATISTICS_HPP

#include <exception>
#include <string>
#include <vector>
#include "../id.hpp"

#include "../clock.hpp"
#include "../threading/ReactionIdentifiers.hpp"

namespace NUClear {
namespace message {

    /**
     * @brief Holds details about reactions that are executed.
     */
    struct ReactionStatistics {

        ReactionStatistics(threading::ReactionIdentifiers identifiers,
                           const NUClear::id_t& reaction_id,
                           const NUClear::id_t& task_id,
                           const NUClear::id_t& cause_reaction_id,
                           const NUClear::id_t& cause_task_id,
                           const clock::time_point& emitted,
                           const clock::time_point& start,
                           const clock::time_point& finish,
                           std::exception_ptr exception)
            : identifiers(std::move(identifiers))
            , reaction_id(reaction_id)
            , task_id(task_id)
            , cause_reaction_id(cause_reaction_id)
            , cause_task_id(cause_task_id)
            , emitted(emitted)
            , started(start)
            , finished(finish)
            , exception(std::move(exception)) {}

        /// @brief A string containing the username/on arguments/and callback name of the reaction.
        threading::ReactionIdentifiers identifiers;
        /// @brief The id of this reaction.
        NUClear::id_t reaction_id{0};
        /// @brief The task id of this reaction.
        NUClear::id_t task_id{0};
        /// @brief The reaction id of the reaction that caused this one or 0 if there was not one
        NUClear::id_t cause_reaction_id{0};
        /// @brief The reaction id of the task that caused this task or 0 if there was not one
        NUClear::id_t cause_task_id{0};
        /// @brief The time that this reaction was emitted to the thread pool
        clock::time_point emitted{};
        /// @brief The time that execution started on this reaction
        clock::time_point started{};
        /// @brief The time that execution finished on this reaction
        clock::time_point finished{};
        /// @brief An exception pointer that can be rethrown (if the reaction threw an exception)
        std::exception_ptr exception{nullptr};
    };

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_REACTIONSTATISTICS_HPP
