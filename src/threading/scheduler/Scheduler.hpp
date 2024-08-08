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
#ifndef NUCLEAR_THREADING_TASK_SCHEDULER_HPP
#define NUCLEAR_THREADING_TASK_SCHEDULER_HPP

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "../../util/GroupDescriptor.hpp"
#include "../../util/ThreadPoolDescriptor.hpp"
#include "../ReactionTask.hpp"
#include "Group.hpp"
#include "Pool.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        class Scheduler {
        public:
            Scheduler(const size_t& thread_count);

            /**
             * Starts the scheduler, and begins executing tasks.
             *
             * The main thread will stay in this function executing tasks until the scheduler is shutdown.
             */
            void start();

            /**
             * Shuts down the scheduler, all waiting threads are woken, and attempting to get a task results in an
             * exception
             */
            void stop();

            /**
             * Submit a new task to be executed to the Scheduler.
             *
             * This method submits a new task to the scheduler. This task will then be sorted into the appropriate
             * queue based on it's sync type and priority. It will then wait there until it is removed by a thread to
             * be processed.
             *
             * @param task the reaction task task to submit
             * @param immediate if this task should run immediately in the current thread. If immediate execution of
             * this task is not possible (e.g. due to group concurrency restrictions) this task will be queued as normal
             */
            void submit(std::unique_ptr<ReactionTask>&& task, const bool& immediate = false) noexcept;

            /**
             * Adds a task to the idle task list.
             *
             * This task will be executed when all pools are idle.
             *
             * @param desc     the descriptor for the pool to add the task to or ALL for the global idle task list
             * @param reaction the reaction to add to the idle task list
             */
            void add_idle_task(const util::ThreadPoolDescriptor& desc, const std::shared_ptr<Reaction>& reaction);

            /**
             * Removes a task from the idle task list.
             *
             * @param desc the descriptor for the pool to remove the task from or ALL for the global idle task list
             * @param id   the reaction id to remove from the idle task list
             */
            void remove_idle_task(const util::ThreadPoolDescriptor& desc, const NUClear::id_t& id);

        private:
            /**
             * Gets a pointer to a specific thread pool, or creates a new one if it does not exist.
             *
             * If the scheduler has already started, this will also start the pool.
             * Otherwise the pool will be started when the scheduler is started.
             *
             * @param desc the descriptor for the pool to get
             *
             * @return a shared pointer to the pool
             */
            std::shared_ptr<Pool> get_pool(const util::ThreadPoolDescriptor& desc);

            /**
             * Gets a pointer to a specific group, or creates a new one if it does not exist.
             *
             * @param desc the descriptor for the group to get
             *
             * @return a shared pointer to the group
             */
            std::shared_ptr<Group> get_group(const util::GroupDescriptor& desc);

            /**
             * Gets a lock object for all the groups listed in the descriptors.
             *
             * @param task_id   the id of the task that is requesting the lock for sorting purposes
             * @param priority  the priority of the task that is requesting the lock for sorting purposes
             * @param pool      the pool to notify when the lock is acquired
             * @param descs     the descriptors for the groups to lock on
             *
             * @return a combined lock representing the state of all the groups
             */
            std::unique_ptr<Lock> get_groups_lock(const NUClear::id_t task_id,
                                                  const int& priority,
                                                  const std::shared_ptr<Pool>& pool,
                                                  const std::set<util::GroupDescriptor>& descs);

            /// If running is false this means the scheduler is shutting down and no more tasks will be accepted
            std::atomic<bool> running{true};
            /// If started is false pools will not be started until start is called
            /// once start is called future pools will be started immediately
            std::atomic<bool> started{false};

            /// A mutex for when we are modifying groups
            std::mutex groups_mutex;
            /// A map of group ids to the number of active tasks currently running in that group
            std::map<NUClear::id_t, std::shared_ptr<Group>> groups{};

            /// A mutex for when we are modifying pools
            std::mutex pools_mutex;
            /// A map of pool descriptor ids to pool descriptors
            std::map<NUClear::id_t, std::shared_ptr<Pool>> pools{};

            /// A mutex to protect the idle tasks list
            std::mutex idle_mutex;
            /// A list of idle tasks to execute when all pools are idle
            std::vector<std::shared_ptr<Reaction>> idle_tasks{};
            /// The number of active thread pools which count for idle
            std::atomic<int> active_pools{0};

            // Pool works with scheduler to manage the thread pools
            friend class Pool;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_TASK_SCHEDULER_HPP
