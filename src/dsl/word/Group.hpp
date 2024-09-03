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

#include "../../threading/ReactionTask.hpp"
#include "../../util/GroupDescriptor.hpp"
#include "../../util/demangle.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * This is used to specify that up to GroupConcurrency reactions in this GroupType can run concurrently.
         *
         * @code on<Trigger<T, ...>, Group<GroupType, N>>() @endcode
         * When a group of tasks has been synchronised, only N task(s) from the group will execute at a given time.
         *
         * Should another task from this group be scheduled/requested (during execution of the current N task(s)), it
         * will be sidelined into the task queue.
         *
         * Tasks in the queue are ordered based on their priority level, then their task id.
         *
         * @attention
         *  When using NUClear, developers should be careful when using devices like a mutex.
         *  In the case of a mutex, threads will run and then block.
         *  This will potentially lead to wasted resources on a number of inactive threads.
         *  By using Sync, NUClear will have task and thread control so system resources can be efficiently managed.
         *
         * @par Implements
         *  Group
         *
         * @tparam GroupType        The type/group to synchronize on.
         *                          This needs to be a declared type within the system.
         *                          It is common to simply use the reactors name for a reactor with one group.
         *                          Should more than one group be required, the developer can declare structs within the
         *                          system, to act as a group reference.
         *                          Note that the developer is not limited to the use of a struct; any declared type
         *                          will work.
         * @tparam GroupConcurrency The number of tasks that should be allowed to run concurrently in this group.
         *                          It is an error to specify a group concurrency less than 1.
         */
        template <typename GroupType>
        struct Group {

            // This must be a separate function, otherwise each instance of DSL will be a separate pool
            static std::shared_ptr<const util::GroupDescriptor> descriptor() {
                static const auto group_descriptor =
                    std::make_shared<const util::GroupDescriptor>(name<GroupType>(), concurrency<GroupType>());
                return group_descriptor;
            }

            template <typename DSL>
            static std::set<std::shared_ptr<const util::GroupDescriptor>> group(
                const threading::ReactionTask& /*task*/) {
                return {descriptor()};
            }

        private:
            template <typename U>
            static auto name() -> decltype(U::name) {
                return U::name;
            }
            template <typename U, typename... A>
            static std::string name(const A&... /*unused*/) {
                return util::demangle(typeid(U).name());
            }

            template <typename U>
            static constexpr auto concurrency() -> decltype(U::concurrency) {
                return U::concurrency;
            }
            template <typename U, typename... A>
            static constexpr int concurrency(const A&... /*unused*/) {
                return 1;
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_GROUP_HPP
