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

#ifndef NUCLEAR_THREADING_TASKSCHEDULER_HPP
#define NUCLEAR_THREADING_TASKSCHEDULER_HPP

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../util/GroupDescriptor.hpp"
#include "../util/ThreadPoolDescriptor.hpp"
#include "Reaction.hpp"
#include "ReactionTask.hpp"

namespace NUClear {
namespace threading {

    /**
     * @brief This class is responsible for scheduling tasks and distributing them amongst threads.
     *
     * @details
     * PRIORITY
     *      what priority this task should run with
     *      tasks are ordered by priority -> creation order
     * POOL
     *      which thread pool this task should execute in
     *      0 being execute on the main thread
     *      1 being the default pool
     * GROUP
     *      which grouping this task belongs to for concurrency (default to the 0 group)
     *      only run if there are less than N tasks running in this group
     * IMMEDIATE
     *      if the submitter of this task should wait until this task is finished before returning (for DIRECT
     * emits)
     *
     *  @em Priority
     *  @code Priority<P> @endcode
     *  When a priority is encountered, the task will be scheduled to execute based on this. If one of the three
     * normal options are specified (HIGH, DEFAULT and LOW), then within the specified Sync group, it will run
     * before, normally or after other reactions.
     *
     *  @em Sync
     *  @code Sync<SyncGroup> @endcode
     *  When a Sync type is encountered, the system uses this as a compile time mutex flag. It will not allow two
     *  callbacks with the same Sync type to execute at the same time. It will effectively ensure that all of the
     *  callbacks with this type run in sequence with each other, rather then in parallel.
     *
     *  @em Group
     *  @code Group<GroupType, GroupConcurrency> @endcode
     *  When a Group type is encountered, the system uses this as a compile time semaphore flag. It will not allow
     *  (GroupConcurrency + 1) callbacks with the same Group type to execute at the same time. It will effectively
     *  ensure that the first GroupConcurrency callbacks with this type run in parallel and all subsequent callbacks
     *  will be queued to run when one of the first GroupConcurrency callbacks have returned
     *
     *  @em Single
     *  @code Single @endcode
     *  If single is encountered while processing the function, and a Task object for this Reaction is already
     * running in a thread, or waiting in the Queue, then this task is ignored and dropped from the system.
     */
    class TaskScheduler {
    public:
        /**
         * @brief Constructs a new TaskScheduler instance, and builds the nullptr sync queue.
         */
        TaskScheduler();

        void start(const size_t& thread_count);

        /**
         * @brief
         *  Shuts down the scheduler, all waiting threads are woken, and any attempt to get a task results in an
         *  exception
         */
        void shutdown();

        /**
         * @brief Submit a new task to be executed to the Scheduler.
         *
         * @details
         *  This method submits a new task to the scheduler. This task will then be sorted into the appropriate
         *  queue based on it's sync type and priority. It will then wait there until it is removed by a thread to
         *  be processed.
         *
         * @param task  the task to be executed
         */
        void submit(std::unique_ptr<ReactionTask>&& task);

    private:
        /**
         * @brief A struct which contains all the information about an individual thread pool
         */
        struct PoolQueue {
            PoolQueue(const util::ThreadPoolDescriptor& pool_descriptor) : pool_descriptor(pool_descriptor) {}
            /// @brief The descriptor for this thread pool
            const util::ThreadPoolDescriptor pool_descriptor;
            /// @brief The threads which are running in this thread pool
            std::vector<std::unique_ptr<std::thread>> threads;
            /// @brief The queue of tasks for this specific thread pool
            std::vector<std::unique_ptr<ReactionTask>> queue;
            /// @brief The mutex which protects the queue
            std::mutex mutex;
            /// @brief The condition variable which threads wait on if they can't get a task
            std::condition_variable condition;
        };

        /**
         * @brief Get a task object to be executed by a thread.
         *
         * @details
         *  This method will get a task object to be executed from the queue. It will block until such a time as a
         *  task is available to be executed. For example, if a task with a particular sync type was out, then this
         *  thread would block until that sync type was no longer out, and then it would take a task.
         *
         * @return the task which has been given to be executed
         */
        std::unique_ptr<ReactionTask> get_task();

        /**
         * @brief Gets a pool queue for the given thread pool descriptor or creates one if it does not exist
         *
         * @param pool the descriptor for the thread pool to get or create
         *
         * @return a shared pointer to the pool queue for the given thread pool descriptor
         */
        std::shared_ptr<PoolQueue> get_pool_queue(const util::ThreadPoolDescriptor& pool);

        /**
         * @brief The function that each thread runs
         *
         * @details This function will repeatedly query the task queue for new a task to run and then execute that task
         *
         * @param pool the thread pool to run from and the task queue to get tasks from
         */
        void pool_func(std::shared_ptr<PoolQueue> pool);

        /**
         * @brief Start all threads for the given thread pool
         */
        void start_threads(const std::shared_ptr<PoolQueue>& pool);

        /**
         * @brief Execute the given task
         *
         * @details After execution of the task has completed the number of active tasks in the tasks' group is
         * decremented
         */
        void run_task(std::unique_ptr<ReactionTask>&& task);

        /**
         * @brief Determines if the given task is able to be executed
         *
         * @details If the current thread is able to be executed the number of active tasks in the tasks' groups is
         * incremented
         *
         * @param task the task to inspect
         *
         * @return true if the task is currently runnable
         * @return false if the task is not currently runnable
         */
        bool is_runnable(const std::unique_ptr<ReactionTask>& task);

        /// @brief if the scheduler is running, and accepting new tasks. If this is false and a new, non-immediate, task
        /// is submitted it will be ignored
        std::atomic<bool> running{true};
        /// @brief if the scheduler has been started. This is set to true after a call to start is made. Once this is
        /// set to true all threads will begin executing tasks from the tasks queue
        std::atomic<bool> started{false};

        /// @brief A map of group ids to the number of active tasks currently running in that group
        std::map<uint64_t, size_t> groups{};
        /// @brief mutex for the group map
        std::mutex group_mutex;

        /// @brief A map of pool descriptor ids to pool descriptors
        std::map<uint64_t, std::shared_ptr<PoolQueue>> pool_queues{};
        /// @brief a mutex for when we are modifying the pool_queues map
        std::mutex pool_mutex;
        /// @brief a pointer to the pool_queue for the current thread so it does not have to access via the map
        static ATTRIBUTE_TLS std::shared_ptr<PoolQueue>* current_queue;
    };

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_TASKSCHEDULER_HPP
