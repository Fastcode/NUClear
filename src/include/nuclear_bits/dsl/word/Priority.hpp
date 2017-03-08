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

#ifndef NUCLEAR_DSL_WORD_PRIORITY_HPP
#define NUCLEAR_DSL_WORD_PRIORITY_HPP

#include "nuclear_bits/threading/Reaction.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  Task priority can be controlled using an assigned setting.
         *
         * @details
         *  @code on<Trigger<T, ...>, Priority::HIGH>() @endcode
         *  The PowerPlant uses this setting to determine the scheduling order (for the associated task) in the
         *  threadpool.
         *
         *  The available priority settings are:
         *
         *  REALTIME
         *  Tasks assigned with this will be queued with all other REALTIME tasks.
         *
         *  HIGH
         *  Tasks assigned with this will be queued with all other HIGH tasks.  They will be scheduled for
         *  execution when there are no REALTIME tasks in the queue.
         *
         *  NORMAL
         *  Tasks assigned with this will be queued with all other NORMAL tasks.  They will be scheduled for
         *  execution when there are no REALTIME or HIGH tasks in the queue.
         *
         *  LOW
         *  Tasks assigned with this will be queued with all other LOW tasks.  They will be scheduled for
         *  execution when there are no REALTIME, HIGH or NORMAL tasks in the queue
         *
         *  IDLE
         *  Tasks assigned with this priority will be queued with all other IDLE tasks.  They will be
         *  scheduled for execution when there are no other tasks running in the system.
         *
         * @par Default Behaviour
         *  @code on<Trigger<T>>() @endcode
         *  When the priority is not assigned, tasks will be generated using the default setting; NORMAL.
         *
         * @attention
         *  How the feature behaves depends on the runtime environments OS scheduling settings.
         *  If the OS does not allow the user to set thread priorities, the executing tasks in NUCLEAR will be
         *  ordered based on their priority setting, but the process will never be given more priority than that which
         *  has already been assigned to the process by the OS.
         *  If the developer wishes to execute more control over process priority, the it is recommended to run
         *  NUCLEAR in a unix environment.  Note that super-users can execute process control using the commands "nice"
         *  and "renice".
         *
         *  For best use, this word should be fused with at least one other binding DSL word.
         *
         * @par Trent??? -
         *  I dont like the attention blurb.
         *  Can I get a small recording on this again and how you are setting the priority on the threadpool?.
         *  "as well as assign a priority to the threadpool"-- I removed this string from the opening para - it seems to
         *  me we are discussing 2 distinct things, so I want to break it out --- and update the attention blurb properly.
         *  I think that string is talking about the priority setting on the threadpool, i.e; within the OS.
         *  I'd like to re-write this section - i think I understand nice and re-nice a bit more, but want to spend a
         *  bit more time going over your input.
         *
         * @par Implements
         *  Fusion
         */
        struct Priority {

            struct REALTIME {
                /// Realtime priority runs with 1000 value
                static constexpr int value = 1000;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };

            struct HIGH {
                /// High priority runs with 750 value
                static constexpr int value = 750;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };

            struct NORMAL {
                /// Normal priority runs with 500 value
                static constexpr int value = 500;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };

            struct LOW {
                /// Low priority runs with 250 value
                static constexpr int value = 250;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };

            struct IDLE {
                /// Idle tasks run with 0 priority, they run when there is free time
                static constexpr int value = 0;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_PRIORITY_HPP
