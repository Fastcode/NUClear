/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_PRIORITY_HPP
#define NUCLEAR_DSL_WORD_PRIORITY_HPP

#include "../../PriorityLevel.hpp"
#include "../../threading/Reaction.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        /**
         * Task priority can be controlled using an assigned setting.
         *
         * @code on<Trigger<T, ...>, Priority::HIGH>() @endcode
         * The PowerPlant uses this setting to determine the scheduling order in the threadpool, as well as assign a
         * priority to the thread on the OS.
         *
         * The available priority settings are:
         *
         * <b>REALTIME:</b>
         * Tasks will attempt to run as soon as possible preempting other threads if possible.
         * Be very careful with this word as once a realtime task is running it will not give up control of its
         * thread until it is finished.
         *
         * <b>HIGH:</b>
         * Tasks assigned with higher priority and will be queued with all other HIGH tasks.
         * Will preempt other tasks in the queue except REALTIME tasks.
         *
         * <b>NORMAL:</b>
         * Tasks assigned with this will be queued with all other NORMAL tasks.
         * Will execute using normal scheduling rules as far as the OS is concerned.
         *
         * <b>LOW:</b>
         * Tasks assigned with this priority will be queued with all other LOW tasks.
         * They may be executed with lower or equal OS thread priority compared to NORMAL tasks.
         *
         * <b>IDLE:</b>
         * Tasks assigned with this priority will be queued with all other IDLE tasks.
         * If possible will be treated as a background task by the OS as well.
         *
         * @par Default Behaviour
         *  @code on<Trigger<T>>() @endcode
         *  When the priority is not specified, tasks will be assigned a default setting; NORMAL.
         *
         * @attention
         *  If the OS allows the user to set thread priority, this word will also be used to assign the priority of the
         *  thread in its runtime environment.
         *
         * @par Implements
         *  Fusion
         */
        struct Priority {
            template <PriorityLevel::Value value>
            struct Value {
                template <typename DSL>
                static PriorityLevel priority(const threading::ReactionTask& /*task*/) {
                    return value;
                }
            };

            using IDLE     = Value<PriorityLevel::IDLE>;
            using LOW      = Value<PriorityLevel::LOW>;
            using NORMAL   = Value<PriorityLevel::NORMAL>;
            using HIGH     = Value<PriorityLevel::HIGH>;
            using REALTIME = Value<PriorityLevel::REALTIME>;
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_PRIORITY_HPP
