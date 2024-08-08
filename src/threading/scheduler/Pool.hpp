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

        class Pool {
        public:
            struct Task {
                /// Holds the task to execute
                std::unique_ptr<ReactionTask> task;
                /// A lock that is held while the task is being executed.
                /// This lock should release via RAII when the task is done.
                std::unique_ptr<Lock> lock;

                /**
                 * Sorts the tasks by the sort order of the reaction tasks
                 *
                 * @param other the other task to compare to
                 *
                 * @return true if this task should be executed before the other task
                 */
                inline bool operator<(const Task& other) const {
                    return *task < *other.task;
                }
            };

            /**
             * Exception thrown when a thread in the pool should shut down.
             */
            class ShutdownThreadException : public std::exception {};

            /**
             * Construct a new thread pool with the given descriptor
             *
             * @param scheduler  the scheduler parent of this pool
             * @param descriptor the descriptor for this thread pool
             */
            explicit Pool(Scheduler& scheduler, const util::ThreadPoolDescriptor& descriptor);

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
             */
            void stop();

            /**
             * Notify a thread in this pool that there is work to do.
             *
             * It will wake up a thread if one is waiting for work, otherwise it will be picked up by the next thread.
             */
            void notify();

            /**
             * Wait for all threads in this pool to exit.
             */
            void join();

            /**
             * Submit a new task to this thread pool
             *
             * @param task the reaction task task to submit
             */
            void submit(Task&& task);

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

        private:
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

            /// The descriptor for this thread pool
            const util::ThreadPoolDescriptor descriptor;

            /// If running is false this means the pool is shutting down and no more tasks will be accepted
            std::atomic<bool> running{true};

            /// The threads which are running in this thread pool
            std::vector<std::unique_ptr<std::thread>> threads;

            /// The mutex which protects the queue and idle tasks
            std::mutex mutex;
            /// The queue of tasks for this specific thread pool
            std::vector<Task> queue;
            /// The condition variable which threads wait on if they can't get a task
            std::condition_variable condition;

            /// The number of active threads in this pool
            std::atomic<IdleLock::semaphore_t> active{0};
            /// The idle tasks for this pool
            std::vector<std::shared_ptr<Reaction>> idle_tasks;

            /// Holds true if the queue has been checked and nothing could run
            bool checked = false;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_POOL_HPP
