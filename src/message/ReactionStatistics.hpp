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
#include <string>
#include <thread>
#include <vector>

#include "../clock.hpp"
#include "../id.hpp"
#include "../threading/ReactionIdentifiers.hpp"
#include "../util/GroupDescriptor.hpp"
#include "../util/ThreadPoolDescriptor.hpp"
#include "../util/usage_clock.hpp"

namespace NUClear {
namespace message {

    /**
     * Holds details about reactions that are executed.
     */
    struct ReactionStatistics {

        struct Event {
            struct ThreadInfo {
                std::thread::id thread_id{};
                util::ThreadPoolDescriptor pool{util::ThreadPoolDescriptor::Invalid()};
            };

            ThreadInfo thread{};
            NUClear::clock::time_point nuclear_time{};
            std::chrono::steady_clock::time_point realtime{};
            util::cpu_clock::time_point cpu_time{};

            static Event now() {

                // Get the thread pool information for this thread if it's a NUClear thread
                Event::ThreadInfo thread_info;

                return Event{
                    thread_info,
                    NUClear::clock::now(),
                    std::chrono::steady_clock::now(),
                    util::cpu_clock::now(),
                };
            }
        };

        ReactionStatistics(std::shared_ptr<threading::ReactionIdentifiers> identifiers,
                           const IDPair& cause,
                           const IDPair& target,
                           const util::ThreadPoolDescriptor& target_threadpool,
                           const std::set<util::GroupDescriptor>& target_groups)
            : identifiers(std::move(identifiers))
            , cause(cause)
            , target(target)
            , target_threadpool(target_threadpool)
            , target_groups(target_groups)
            , created(Event::now()) {};


        /// The identifiers for the reaction that was executed
        std::shared_ptr<threading::ReactionIdentifiers> identifiers;

        /// The reaction/task pair that caused this reaction or 0s if it was a non NUClear cause
        IDPair cause;
        /// The reaction/task pair that was executed
        IDPair target;

        /// The thread pool that this reaction was intended to run on
        util::ThreadPoolDescriptor target_threadpool;
        /// The group that this reaction was intended to run in
        std::set<util::GroupDescriptor> target_groups;

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
        enum Event { CREATED, MISSING_DATA, BLOCKED, STARTED, FINISHED };

        ReactionEvent(const Event& type, std::shared_ptr<ReactionStatistics> statistics)
            : type(type), statistics(std::move(statistics)) {}

        Event type;
        std::shared_ptr<ReactionStatistics> statistics;
    };

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_REACTION_STATISTICS_HPP
