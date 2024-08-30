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
#ifndef NUCLEAR_THREADING_SCHEDULER_POOL_HPP
#define NUCLEAR_THREADING_SCHEDULER_POOL_HPP

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../../util/ThreadPoolDescriptor.hpp"
#include "../ReactionTask.hpp"
#include "Lock.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        // Forward declare the scheduler
        class Scheduler;

        class Pool : public std::enable_shared_from_this<Pool> {
        public:
            enum class StopType : uint8_t {
                /// Normal stop, wait for all tasks to finish and accept no more tasks
                /// Pools which ignore shutdown will continue to accept tasks
                NORMAL,
                /// Final stop request, pools which ignore shutdown will finish when all tasks are done
                /// However they will continue to accept tasks
                FINAL,
                /// Force stop, the queue will be cleared and all threads will be woken
                FORCE
            };

            struct Task {
                /**
                 * @brief Construct a new Task object
                 *
                 * @param task the task to execute
                 * @param lock the RAII lock to hold while the task is being executed
                 */
                Task(std::unique_ptr<ReactionTask>&& task = nullptr, std::unique_ptr<Lock>&& lock = nullptr)
                    : task(std::move(task)), lock(std::move(lock)) {}

                /// Holds the task to execute
                std::unique_ptr<ReactionTask> task;
                /// A lock that is held while the task is being executed.
                /// This lock should release via RAII when the task is done.
                std::unique_ptr<Lock> lock;

                /**
                 * Sorts the tasks by the sort order of the reaction tasks
                 *
                 * @param lhs The left hand side task
                 * @param rhs The right hand side task
                 *
                 * @return true if this task should be executed before the other task
                 */
                friend bool operator<(const Task& lhs, const Task& rhs) {
                    return *lhs.task < *rhs.task;
                }
            };

            /**
             * Construct a new thread pool with the given descriptor
             *
             * @param scheduler  the scheduler parent of this pool
             * @param descriptor the descriptor for this thread pool
             */
            explicit Pool(Scheduler& scheduler, std::shared_ptr<const util::ThreadPoolDescriptor> descriptor);

            // No moving or copying
            Pool(const Pool&)            = delete;
            Pool(Pool&&)                 = delete;
            Pool& operator=(const Pool&) = delete;
            Pool& operator=(Pool&&)      = delete;

            /**
             * Destroy the Pool object
             *
             * Will stop the pool if it is still running and wait for all threads to exit.
             */
            ~Pool();

            /**
             * Starts the thread pool and begins executing tasks.
             *
             * If the main thread pool is started then the main thread will stay in this function executing tasks until
             * the scheduler is shutdown.
             */
            void start();

            /**
             * Stops the thread pool, all threads are woken and once the task queue is empty the threads will exit.
             * This function returns immediately, use join to wait for the threads to exit.
             *
             * @param type the type of stop to perform
             */
            void stop(const StopType& type);

            /**
             * Notify a thread in this pool that there is work to do.
             *
             * It will wake up a thread if one is waiting for work, otherwise it will be picked up by the next thread.
             *
             * @param clear_idle If true, the idle state of the pool will be cleared
             */
            void notify(bool clear_idle);

            /**
             * Wait for all threads in this pool to exit.
             */
            void join() const;

            /**
             * Submit a new task to this thread pool
             *
             * @param task       The reaction task task to submit
             * @param clear_idle If true, the idle state of the pool will be cleared
             */
            void submit(Task&& task, bool clear_idle);

            /**
             * Add an idle task to this pool.
             *
             * This will add a task to the idle task list for this pool.
             *
             * @param reaction the reaction to add to the idle task list
             */
            void add_idle_task(const std::shared_ptr<Reaction>& reaction);

            /**
             * Remove an idle task from this pool.
             *
             * @param id the id of the reaction to remove from the idle task list
             */
            void remove_idle_task(const NUClear::id_t& id);

            /**
             * Returns the thread pool that the current thread is running in, or nullptr if the current thread is not a
             * scheduler thread.
             *
             * @return the thread pool that the current thread is running in
             */
            static std::shared_ptr<Pool> current();

            /**
             * Returns if the current thread is idle.
             *
             * @return true if the current thread is idle
             */
            bool is_idle() const;

            /// The descriptor for this thread pool
            const std::shared_ptr<const util::ThreadPoolDescriptor> descriptor;

        private:
            /**
             * Exception thrown when a thread in the pool should shut down.
             */
            class ShutdownThreadException : public std::exception {};

            /**
             * The main function executed by each thread in the pool.
             *
             * The thread will wait for a task to be available and then execute it.
             * This will continue until the pool is stopped.
             */
            void run();

            /**
             * Get the next task to execute.
             *
             * This will return the next task to execute or block until a task is available.
             *
             * @return the next task to execute
             */
            Task get_task();

            /**
             * Get an idle task to execute or hold.
             *
             * This will return an idle task instance.
             * If the lock on the idle task returns true, it will then execute the idle task which enqueues the tasks
             * that have been declared.
             *
             * If this was not the last thread to become idle, it will return an object which will not lock and the
             * thread should then sleep until it is woken.
             *
             * @return the idle task to execute if it is lockable or hold if it is not
             */
            Task get_idle_task();

            // The scheduler parent of this pool
            Scheduler& scheduler;

            /// If running is false this means the pool is shutting down and no more tasks will be accepted
            bool running = true;
            /// If accept is false this pool will no longer accept new tasks
            bool accept = true;

            /// The threads which are running in this thread pool
            std::vector<std::unique_ptr<std::thread>> threads;

            /// The queue of tasks for this specific thread pool
            std::vector<Task> queue;
            /// A boolean which is set to true when the queue is modified and set to false when there was no work to do
            bool live = true;
            /// The mutex which protects the queue and idle tasks
            mutable std::mutex mutex;
            /// The condition variable which threads wait on if they can't get a task
            std::condition_variable condition;

            /// The number of active threads in this pool
            std::atomic<int> active{0};
            /// The idle tasks for this pool
            std::vector<std::shared_ptr<Reaction>> idle_tasks;

            /// The lock which holds the idle state for the specific thread in the pool
            std::map<std::thread::id, std::unique_ptr<Lock>> thread_idle;

            /// When this lock is held, the pool is considered idle
            /// The idle status will be removed when a non idle task is retrieved from the queue
            /// Or when another thread pool notifies this pool, giving its chance at global idle to this pool
            std::unique_ptr<Lock> pool_idle = nullptr;

            /// A thread local pointer to the current pool this thread is running in
            static ATTRIBUTE_TLS Pool* current_pool;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

            friend class Scheduler;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_POOL_HPP
