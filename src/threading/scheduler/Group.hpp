/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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
#ifndef NUCLEAR_THREADING_SCHEDULER_GROUP_HPP
#define NUCLEAR_THREADING_SCHEDULER_GROUP_HPP

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "../../util/GroupDescriptor.hpp"
#include "Lock.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        /**
         * A group is a collection of tasks which are mutually exclusive to each other.
         *
         * They are identified by having a common group id along with a maximum concurrency.
         * This class holds the structures that manage the group.
         *
         * This class is used along with the GroupLock class to manage the group locking.
         */
        class Group {

        private:
            /**
             * A lock handle holds the shared state between the group object and the lock objects.
             * It holds if the lock should currently be locked, as well as ordering which locks should be locked first.
             */
            struct LockHandle {
                LockHandle(const NUClear::id_t& task_id, const int& priority, std::function<void()> notify);

                /**
                 * Compare two lock handles by comparing their priority and task id
                 *
                 * @param other the other lock handle to compare to
                 *
                 * @return true if this lock handle should execute before the other
                 */
                friend bool operator<(const LockHandle& lhs, const LockHandle& rhs) {
                    return lhs.priority == rhs.priority ? lhs.task_id < rhs.task_id : lhs.priority > rhs.priority;
                }

                /**
                 * Compare two shared pointers to lock handles by comparing the lock handles they point to
                 *
                 * @param a the first shared pointer to compare
                 * @param b the second shared pointer to compare
                 *
                 * @return true if the task pointed to by the first lock handle should execute before the other
                 */
                friend bool operator<(const std::shared_ptr<LockHandle>& a, const std::shared_ptr<LockHandle>& b) {
                    return *a < *b;
                }

                /// The task id of the reaction that is waiting, lower task ids run first
                NUClear::id_t task_id;
                /// The priority of the reaction that is waiting, higher priorities run first
                int priority;
                /// If this lock has been successfully locked
                bool locked{false};
                /// If this lock has been notified that it can lock
                bool notified{false};
                /// The function to execute when this lock is able to be locked
                std::function<void()> notify;
            };

        public:
            /**
             * A group lock is the RAII lock object that is used by the Pools to manage the group locking.
             */
            class GroupLock : public Lock {
            public:
                /**
                 * Construct a new Group Lock object
                 *
                 * @param group  the reference to the group this lock is for
                 * @param handle the shared state between the group object and the lock objects
                 */
                GroupLock(Group& group, std::shared_ptr<LockHandle> handle);

                // Not movable or copyable
                GroupLock(const GroupLock&)            = delete;
                GroupLock(GroupLock&&)                 = delete;
                GroupLock& operator=(const GroupLock&) = delete;
                GroupLock& operator=(GroupLock&&)      = delete;

                /**
                 * Destroy the Group Lock object
                 *
                 * Releases the lock on the group and notifies the next task in the queue
                 */
                ~GroupLock() override;

                /**
                 * Locks the group for this task.
                 *
                 * Before this function is called, if a higher priority task enters the queue it may obtain the lock
                 * before this task. However once this task has successfully returned true from this function it will
                 * have the lock until it is destroyed.
                 *
                 * @return true if the lock was successfully obtained
                 */
                bool lock() override;

            private:
                /// The group this lock is for
                Group& group;
                /// The shared state between the group object and the lock objects
                std::shared_ptr<LockHandle> handle;
            };

            /**
             * Construct a new Group object
             *
             * @param descriptor The descriptor for this group
             */
            explicit Group(std::shared_ptr<const util::GroupDescriptor> descriptor);

            /**
             * This function will create a new lock for the task and return it.
             *
             * This lock will have its lock() function return true once a token has been assigned to it.
             * The tokens will be assigned in the same priority order which would apply to the task if it were in a
             * single queue with all other tasks of the same group.
             *
             * If a higher priority task comes in before this task calls lock() then the higher priority task may take
             * the token instead.
             *
             * @param task_id   The id of the task that is waiting. used for sorting
             * @param priority  The priority of the task that is waiting. used for sorting
             * @param notify    The function to execute when this lock is ready
             *
             * @return a lock which can be locked once a token is available
             */
            std::unique_ptr<Lock> lock(const NUClear::id_t& task_id,
                                       const int& priority,
                                       const std::function<void()>& notify);

            /// The descriptor for this group
            const std::shared_ptr<const util::GroupDescriptor> descriptor;

        private:
            /// The mutex which protects the queue
            std::mutex mutex;
            /// The number of tokens that are available for this group
            int tokens = descriptor->concurrency;
            /// The queue of tasks for this specific thread pool and if they are group blocked
            std::vector<std::shared_ptr<LockHandle>> queue;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_GROUP_HPP
