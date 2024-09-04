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

#ifndef NUCLEAR_MESSAGE_REACTION_STATISTICS_HPP
#define NUCLEAR_MESSAGE_REACTION_STATISTICS_HPP

#include <exception>
#include <memory>
#include <set>
#include <string>
#include <thread>

#include "../clock.hpp"
#include "../id.hpp"
#include "../util/usage_clock.hpp"

namespace NUClear {

// Forward declarations
namespace util {
    struct ThreadPoolDescriptor;
    struct GroupDescriptor;
}  // namespace util
namespace threading {
    struct ReactionIdentifiers;
}  // namespace threading

namespace message {

    /**
     * Holds details about reactions that are executed.
     */
    struct ReactionStatistics {

        struct Event {
            struct ThreadInfo {
                /// The thread id that this event occurred on
                std::thread::id thread_id;
                /// The thread pool that this event occurred on or nullptr if it was not on a thread pool
                std::shared_ptr<const util::ThreadPoolDescriptor> pool = nullptr;
            };

            /// The thread that this event occurred on
            ThreadInfo thread{};
            /// The time that this event occurred in NUClear time
            NUClear::clock::time_point nuclear_time;
            /// The time that this event occurred in real time
            std::chrono::steady_clock::time_point real_time;
            /// The time that this event occurred in CPU thread time
            util::cpu_clock::time_point thread_time;

            /**
             * Creates a new Event object with the current time and thread information.
             *
             * @return The new Event object.
             */
            static Event now();
        };

        ReactionStatistics(std::shared_ptr<const threading::ReactionIdentifiers> identifiers,
                           const IDPair& cause,
                           const IDPair& target,
                           std::shared_ptr<const util::ThreadPoolDescriptor> target_pool,
                           std::set<std::shared_ptr<const util::GroupDescriptor>> target_groups);

        /// The identifiers for the reaction that was executed
        std::shared_ptr<const threading::ReactionIdentifiers> identifiers;

        /// The reaction/task pair that caused this reaction or 0s if it was a non NUClear cause
        IDPair cause;
        /// The reaction/task pair that was executed
        IDPair target;

        /// The thread pool that this reaction was intended to run on
        std::shared_ptr<const util::ThreadPoolDescriptor> target_pool;
        /// The groups that this reaction was intended to run in
        std::set<std::shared_ptr<const util::GroupDescriptor>> target_groups;

        /// The time and thread information for when this reaction was created
        Event created;
        /// The time and thread information for when this reaction was started
        Event started;
        /// The time and thread information for when this reaction was finished
        Event finished;

        /// The exception that was thrown by this reaction or nullptr if no exception was thrown
        std::exception_ptr exception;
    };

    struct ReactionEvent {
        enum Event : uint8_t { CREATED, MISSING_DATA, BLOCKED, STARTED, FINISHED };

        ReactionEvent(const Event& type, std::shared_ptr<const ReactionStatistics> statistics)
            : type(type), statistics(std::move(statistics)) {}

        Event type;
        std::shared_ptr<const ReactionStatistics> statistics;
    };

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_REACTION_STATISTICS_HPP
