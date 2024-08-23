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

#ifndef NUCLEAR_DSL_WORD_SYNC_HPP
#define NUCLEAR_DSL_WORD_SYNC_HPP

#include <map>
#include <mutex>
#include <typeindex>

#include "../../threading/ReactionTask.hpp"
#include "../../util/GroupDescriptor.hpp"
#include "Group.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * This is used to specify that only one reaction in this SyncGroup can run concurrently.
         *
         *
         * @code on<Trigger<T, ...>, Sync<SyncGroup>>() @endcode
         * When a group of tasks has been synchronised, only one task from the group will execute at a given time.
         *
         * Should another task from this group be scheduled/requested (during execution of the current task), it will
         * be sidelined into the task queue.
         *
         * Tasks in the queue are ordered based on their priority level, then their task id.
         *
         * @par When should I use Sync
         *  Consider a reactor with a number of a reactions which modify its state.
         *  It would be unwise to allow the reactions to run concurrently.
         *  To avoid race conditions, it is recommended that any reactions which modify the state be synced.
         *
         * @attention
         *  When using NUClear, developers should avoid using blocking features such as mutexes.
         *  In the case of a mutex, threads  will run and then block.
         *  As they are using a thread pool, this can lead to wasted resources by the of inactive threads.
         * By using Sync, NUClear will have task and thread control so that system resources can be efficiently managed.
         *
         * @par Implements
         *  Group
         *
         * @tparam GroupType The type/group to synchronize on.
         *                   This needs to be a declared type within the system.
         *                   It is common to simply use the reactors name for a reactor with one group.
         *                   Should more than one group be required, the developer can declare structs within the
         *                   system, to act as a group reference.
         */
        template <typename SyncGroup>
        struct Sync : Group<SyncGroup> {};

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_SYNC_HPP
