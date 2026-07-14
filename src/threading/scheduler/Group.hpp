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

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "../../PriorityLevel.hpp"
#include "../../util/GroupDescriptor.hpp"
#include "Lock.hpp"
#include "Pool.hpp"
#include "queue/Priority.hpp"
#include "queue/TaskQueue.hpp"

namespace NUClear {
namespace threading {

    class ReactionTask;

    namespace scheduler {

        class Pool;

        /**
         * A group is a collection of tasks which are mutually exclusive to each other.
         *
         * They are identified by having a common group id along with a maximum concurrency.
         * This class holds the structures that manage the group.
         *
         * Tasks submitted through the scheduler fast path use lock-free waiter buckets.
         * The lock() API uses a mutex-protected sorted queue for multi-group and unit-test use.
         */
        class Group : public std::enable_shared_from_this<Group> {

        private:
            struct WaitEntry {
                std::unique_ptr<ReactionTask> task;
                /// Non-owning pointer; Pools live for the lifetime of the Scheduler and the
                /// Scheduler tears down Groups before Pools, so it is always safe to dereference
                /// while this WaitEntry is reachable.
                Pool* pool{nullptr};
                bool clear_idle{false};
                /// Single-use arbiter shared between this waiter's own park_reconcile() and any
                /// pre-paying drainer (the GroupLock opportunistic drain). Both attempt to flip it
                /// from false to true; whoever wins is the party that performs the waiter's single
                /// token decrement, and the loser skips its own adjustment. This makes the
                /// keep/hand-back decision exact regardless of how many other waiters are parked,
                /// instead of inferring it from the (unreliable) emptiness of the wait buckets.
                std::shared_ptr<std::atomic<bool>> slot;
                /// Keeps the destination pool's workers alive until this entry is drained or destroyed.
                ExternalWaiterRegistration external_waiter;
            };

            struct DrainResult {
                bool drained{false};
                /// True when the drained waiter had not yet accounted its own token (its arbiter slot
                /// was still false and this drain claimed it). The caller is then responsible for the
                /// waiter's single token decrement; for an already-counted waiter the drain is
                /// token-neutral.
                bool uncounted{false};
            };

            /**
             * A lock handle holds the shared state between the group object and the lock objects.
             * It holds if the lock should currently be locked, as well as ordering which locks should be locked first.
             */
            struct LockHandle {
                LockHandle(const NUClear::id_t& task_id, const PriorityLevel& priority, std::function<void()> notify);

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
                PriorityLevel priority;
                /// If this lock has been successfully locked
                bool locked{false};
                /// If this lock has been notified that it can lock
                bool notified{false};
                /// The function to execute when this lock is able to be locked
                std::function<void()> notify;
            };

            /**
             * RAII lock released when a fast-path task finishes executing.
             */
            class RunningLock : public Lock {
            public:
                RunningLock(Group& group, std::shared_ptr<Group> group_keepalive);
                ~RunningLock() override;

                RunningLock(const RunningLock&)            = delete;
                RunningLock(RunningLock&&)                 = delete;
                RunningLock& operator=(const RunningLock&) = delete;
                RunningLock& operator=(RunningLock&&)      = delete;

                bool lock() override;

            private:
                Group& group;
                std::shared_ptr<Group> keepalive;
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
             * Destroy the Group object. Drains any parked waiters in the fast-path buckets so the
             * `external_waiters` counter on every Pool referenced by a queued WaitEntry is balanced
             * back to zero; otherwise a Pool worker could spin forever in `get_task()` waiting for
             * waiters that will never be drained.
             */
            ~Group();

            Group(const Group&)            = delete;
            Group(Group&&)                 = delete;
            Group& operator=(const Group&) = delete;
            Group& operator=(Group&&)      = delete;

            /**
             * Try to acquire a token for inline execution without submitting to a pool.
             *
             * @return an RAII lock if a token was acquired, otherwise nullptr
             */
            std::unique_ptr<Lock> try_acquire_running_lock();

            /**
             * Try to submit a task through the lock-free fast path.
             *
             * If a group token is available the task is submitted to the pool immediately.
             * Otherwise the task is queued until a token is released.
             *
             * @param task       the reaction task to submit
             * @param pool       the pool to submit to when runnable (non-owning; must outlive the call)
             * @param clear_idle if true, clear idle state on submission
             *
             * @return true if the task was submitted immediately
             */
            bool try_submit(std::unique_ptr<ReactionTask>&& task, Pool* pool, const bool& clear_idle);

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
                                       const PriorityLevel& priority,
                                       const std::function<void()>& notify);

            /// The descriptor for this group
            const std::shared_ptr<const util::GroupDescriptor> descriptor;

        private:
            std::shared_ptr<std::atomic<bool>> park_publish(std::unique_ptr<ReactionTask>&& task,
                                                            Pool* pool,
                                                            const bool& clear_idle) noexcept;
            void park_reconcile(const std::shared_ptr<std::atomic<bool>>& slot) noexcept;
            void release_token() noexcept;
            void notify_slow_path() noexcept;
            DrainResult drain_one_to_pool() noexcept;
            std::unique_ptr<Lock> make_running_lock();

            /// Available group tokens (signed when waiters are queued on the fast path)
            std::atomic<int> tokens;
            /// Number of unsatisfied slow-path waiters
            std::atomic<int> slow_pending{0};
            /// Lock-free wait queues keyed by priority
            std::array<queue::TaskQueue<WaitEntry>, queue::PRIORITY_BUCKETS> wait_buckets;

            /// The mutex which protects the slow-path queue
            std::mutex mutex;
            /// The queue of tasks for the slow path
            std::vector<std::shared_ptr<LockHandle>> queue;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_GROUP_HPP
