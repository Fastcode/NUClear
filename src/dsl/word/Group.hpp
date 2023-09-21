/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_GROUP_HPP
#define NUCLEAR_DSL_WORD_GROUP_HPP

#include <map>
#include <mutex>
#include <typeindex>

#include "../../threading/ReactionTask.hpp"
#include "../../util/GroupDescriptor.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This is used to specify that up to GroupConcurrency reactions in this GroupType can run concurrently.
         *
         * @details
         *  @code on<Trigger<T, ...>, Group<GroupType, N>>() @endcode
         *  When a group of tasks has been synchronised, only N task(s) from the group will execute at a given time.
         *
         *  Should another task from this group be scheduled/requested (during execution of the current N task(s)), it
         *  will be sidelined into the task queue.
         *
         *  Tasks in the queue are ordered based on their priority level, then their task id.
         *
         * @attention
         *  When using NUClear, developers should be careful when using devices like a mutex. In the case of a mutex,
         *  threads will run and then block (leading to wasted resources on a number of inactive threads).  By using
         *  Sync, NUClear will have task and thread control so that system resources can be efficiently managed.
         *
         * @par Implements
         *  Group
         *
         * @tparam GroupType
         *  the type/group to synchronize on.  This needs to be a declared type within the system.  It is common to
         *  simply use the reactors name (i.e; if the reactor is only syncing with one group).  Should more than one
         *  group be required, the developer can declare structs within the system, to act as a group reference.
         *  Note that the developer is not limited to the use of a struct; any declared type will work.
         * @tparam GroupConcurrency
         *  the number of tasks that should be allowed to run concurrently in this group.  It is an error to specify a
         *  group concurrency less than 1.
         */
        template <typename GroupType, int GroupConcurrency = 1>
        struct Group {

            static_assert(GroupConcurrency > 0, "Can not have a group with concurrency less than 1");

            static const util::GroupDescriptor group_descriptor;

            template <typename DSL>
            static inline util::GroupDescriptor group(const threading::Reaction& /*reaction*/) {
                return group_descriptor;
            }
        };

        // Initialise the group descriptor
        template <typename GroupType, int GroupConcurrency>
        const util::GroupDescriptor Group<GroupType, GroupConcurrency>::group_descriptor = {
            util::GroupDescriptor::get_unique_group_id(),
            GroupConcurrency};

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_GROUP_HPP
