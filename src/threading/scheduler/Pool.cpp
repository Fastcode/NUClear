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
#include "Pool.hpp"

#include "../../message/ReactionStatistics.hpp"
#include "../ReactionTask.hpp"
#include "CombinedLock.hpp"
#include "CountingLock.hpp"
#include "Scheduler.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        Pool::Pool(Scheduler& scheduler, const util::ThreadPoolDescriptor& descriptor)
            : scheduler(scheduler), descriptor(descriptor) {}

        Pool::~Pool() {

            // Stop the pool threads and wait for them to finish
            stop();
            join();

            // One less active pool
            scheduler.active_pools.fetch_sub(descriptor.counts_for_idle ? 1 : 0, std::memory_order_relaxed);
        }

        void Pool::start() {
            // Set the number of active threads to the number of threads in the pool
            active = descriptor.counts_for_idle ? descriptor.thread_count : 0;

            // Increase the number of active pools if this pool counts for idle
            scheduler.active_pools.fetch_add(descriptor.counts_for_idle ? 1 : 0, std::memory_order_relaxed);

            // The main thread never needs to be started
            if (descriptor.pool_id != NUClear::id_t(util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID)) {
                const std::lock_guard<std::mutex> lock(mutex);
                while (threads.size() < descriptor.thread_count) {
                    threads.emplace_back(std::make_unique<std::thread>(&Pool::run, this));
                }
            }
            else {
                // The main thread is the current thread so we can just run it
                run();
            }
        }

        void Pool::stop() {
            // Stop the pool threads
            std::lock_guard<std::mutex> lock(mutex);
            running = false;
            live    = true;
            condition.notify_all();
        }

        void Pool::notify() {
            std::lock_guard<std::mutex> lock(mutex);
            /// May not be idle anymore, flag this before the thread wakes up
            live      = true;
            pool_idle = nullptr;
            condition.notify_one();
        }

        void Pool::join() {
            // Join all the threads
            for (auto& thread : threads) {
                if (thread->joinable()) {
                    thread->join();
                }
            }
        }

        void Pool::submit(Task&& task) {
            const std::lock_guard<std::mutex> lock(mutex);

            // Insert in sorted order
            queue.insert(std::lower_bound(queue.begin(), queue.end(), task), std::move(task));

            // Pool might have something to do now
            live = true;

            // Notify a single thread that there is a new task
            condition.notify_one();
        }

        void Pool::add_idle_task(const std::shared_ptr<Reaction>& reaction) {
            const std::lock_guard<std::mutex> lock(mutex);
            idle_tasks.push_back(reaction);

            // If we previously had no idle tasks, it's possible every thread is sleeping (idle)
            // Wake one up so that it can check again
            if (idle_tasks.size() == 1) {
                condition.notify_one();
            }
        }

        void Pool::remove_idle_task(const NUClear::id_t& id) {
            const std::lock_guard<std::mutex> lock(mutex);
            idle_tasks.erase(
                std::remove_if(idle_tasks.begin(), idle_tasks.end(), [&](const auto& r) { return r->id == id; }),
                idle_tasks.end());
        }

        void Pool::run() {

            // When task is nullptr there are no more tasks to get and the scheduler is shutting down
            while (true) {
                try {
                    // Run the next task
                    Task task = get_task();
                    task.task->run();
                }
                catch (const ShutdownThreadException&) {
                    // This thread is shutting down
                    return;
                }
                catch (...) {
                }
            }
        }

        Pool::Task Pool::get_task() {

            std::unique_lock<std::mutex> lock(mutex);
            while (running || !queue.empty()) {
                if (live) {
                    // Get the first task that can be run
                    for (auto it = queue.begin(); it != queue.end(); ++it) {
                        // If the task is not a group member, or we can get a token for the group then we can run it
                        if (it->lock == nullptr || it->lock->lock()) {
                            // If the task is not group blocked or we can lock the group then we can run it
                            Task task = std::move(*it);
                            queue.erase(it);
                            thread_idle[std::this_thread::get_id()] = nullptr;  // This thread is no longer idle
                            pool_idle                               = nullptr;  // The pool as a whole is no longer idle
                            return task;
                        }
                    }
                }
                live = false;

                auto idle_task = get_idle_task();
                if (idle_task.task != nullptr) {
                    return idle_task;
                }

                // Wait for something to happen!
                condition.wait(lock, [this] { return live || (!running && queue.empty()); });
            }

            condition.notify_all();
            throw ShutdownThreadException();
        }

        Pool::Task Pool::get_idle_task() {
            // Don't idle when shutting down, don't idle if we can't idle, don't idle if we are already idle
            if (!running || !descriptor.counts_for_idle) {
                return Task{};
            }

            // Tasks to be executed when idle
            std::vector<std::shared_ptr<Reaction>> tasks;

            /// Current local lock status
            auto& local_lock = thread_idle[std::this_thread::get_id()];

            // If not already idle, check to see if we are the last and if so add the local idle tasks
            if (local_lock == nullptr) {
                local_lock = std::make_unique<CountingLock>(active);
                if (local_lock->lock()) {
                    tasks.insert(tasks.end(), idle_tasks.begin(), idle_tasks.end());
                }
            }

            // The if the pool is idle and does not have a global idle task, try the global lock
            if (pool_idle == nullptr && active == 0) {
                pool_idle = std::make_unique<CountingLock>(scheduler.active_pools);

                // This was the last pool to become idle, so get the global idle tasks
                if (pool_idle->lock()) {
                    std::lock_guard<std::mutex> lock(scheduler.idle_mutex);
                    tasks.insert(tasks.end(), scheduler.idle_tasks.begin(), scheduler.idle_tasks.end());
                }
            }

            // If there are no idle tasks, return no task
            if (tasks.empty()) {
                return Task{};
            }

            // Make a reaction task which will submit all the idle tasks to the scheduler
            auto task = std::make_unique<ReactionTask>(
                nullptr,
                [](const ReactionTask&) { return 0; },
                [](const ReactionTask&) { return util::ThreadPoolDescriptor{}; },
                [](const ReactionTask&) { return std::set<util::GroupDescriptor>{}; });
            task->callback = [this, tasks = std::move(tasks)](const ReactionTask& /*task*/) {
                for (auto& idle_task : tasks) {
                    // Submit all the idle tasks to the scheduler
                    scheduler.submit(idle_task->get_task(), false);
                }
            };

            return Task{std::move(task)};
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
