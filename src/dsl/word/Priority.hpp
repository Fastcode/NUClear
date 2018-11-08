/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include "../../threading/Reaction.hpp"

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
         *  threadpool, as well as assign a priority to the thread.
         *
         *  The available priority settings are:
         *
         *  <b>REALTIME:</b>  Tasks assigned with this will be queued with all other REALTIME tasks.
         *
         *  <b>HIGH:</b>  Tasks assigned with this will be queued with all other HIGH tasks.  They will be scheduled for
         *  execution when there are no REALTIME tasks in the queue.
         *
         *  <b>NORMAL:</b> Tasks assigned with this will be queued with all other NORMAL tasks.  They will be scheduled
         *  for execution when there are no REALTIME and HIGH tasks in the queue.
         *
         *  <b>LOW:</b>  Tasks assigned with this will be queued with all other LOW tasks.  They will be scheduled for
         *  execution when there are no REALTIME, HIGH and NORMAL tasks in the queue.
         *
         *  <b>IDLE:</b>  Tasks assigned with this priority will be queued with all other IDLE tasks.  They will be
         *  scheduled for execution when there are no other tasks running in the system.
         *
         *  For best use, this word should be fused with at least one other binding DSL word.
         *
         * @par Default Behaviour
         *  @code on<Trigger<T>>() @endcode
         *  When the priority is not specified, tasks will be assigned a default setting; NORMAL.
         *
         * @attention
         *  If the OS allows the user to set thread priority, this word can also be used to assign the priority of the
         *  thread in its runtime environment.
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
