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
#include "IdleLock.hpp"
#include "Scheduler.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        Pool::Pool(Scheduler& scheduler, const util::ThreadPoolDescriptor& descriptor)
            : scheduler(scheduler), descriptor(descriptor) {}

        void Pool::start() {
            // Increase the number of active threads if this pool counts for idle
            active = descriptor.counts_for_idle ? descriptor.thread_count : 0;
            scheduler.active.fetch_add(active, std::memory_order_relaxed);

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
            running.store(false, std::memory_order_relaxed);
            std::lock_guard<std::mutex> lock(mutex);
            condition.notify_all();
        }

        void Pool::notify() {
            std::lock_guard<std::mutex> lock(mutex);
            checked = false;
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
            checked = false;

            // Notify a single thread that there is a new task
            condition.notify_one();
        }

        void Pool::add_idle_task(const std::shared_ptr<Reaction>& reaction) {
            const std::lock_guard<std::mutex> lock(mutex);
            idle_tasks.push_back(reaction);

            // If we previously had no idle tasks, it's possible every thread is sleeping (idle)
            // Therefore we need to notify all since we don't know which thread is the one that holds the idle lock
            if (idle_tasks.size() == 1) {
                checked = false;
                condition.notify_all();
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
            while (running.load() || !queue.empty()) {
                try {
                    // Run the next task
                    Task task = get_task();
                    task.task->run();
                }
                catch (...) {
                }
            }
        }

        Pool::Task Pool::get_task() {

            std::unique_lock<std::mutex> lock(mutex);
            while (running.load(std::memory_order_relaxed) || !queue.empty()) {
                if (!checked) {
                    // Get the first task that can be run
                    for (auto it = queue.begin(); it != queue.end(); ++it) {
                        // If the task is not a group member, or we can get a token for the group then we can run it
                        if (it->lock == nullptr || it->lock->lock()) {
                            // If the task is not group blocked or we can lock the group then we can run it
                            Task task = std::move(*it);
                            queue.erase(it);
                            return task;
                        }
                    }
                    checked = true;
                }

                // If we reach here before we sleep check if we can run the idle task
                auto idle_task = get_idle_task();
                if (idle_task.task != nullptr && idle_task.lock->lock()) {
                    return idle_task;
                }

                // Wait for something to happen!
                condition.wait(lock, [this] { return !checked || (!running.load() && queue.empty()); });
            }

            condition.notify_all();
            throw ShutdownThreadException();
        }

        Pool::Task Pool::get_idle_task() {
            // If this pool does not count for idle, it can't participate in idle tasks
            if (!running.load(std::memory_order_relaxed) || !descriptor.counts_for_idle) {
                return Task{nullptr, nullptr};
            }

            // Make the idle lock to which will make this thread count as idle
            auto idle_lock = std::make_unique<IdleLockPair>(active, scheduler.active);

            // If we weren't the last, just return no task along with the lock to hold the idle state
            if (!idle_lock->lock()) {
                return Task{nullptr, std::move(idle_lock)};
            }

            std::vector<std::shared_ptr<Reaction>> tasks;
            if (idle_lock->local_lock()) {
                tasks.insert(tasks.end(), idle_tasks.begin(), idle_tasks.end());
            }
            if (idle_lock->global_lock()) {
                // TODO unprotected access to scheduler idle tasks here
                tasks.insert(tasks.end(), scheduler.idle_tasks.begin(), scheduler.idle_tasks.end());
            }

            // If there are no idle tasks, return no task along with the lock to hold the idle state
            if (tasks.empty()) {
                return Task{nullptr, std::move(idle_lock)};
            }

            auto task = std::make_unique<ReactionTask>(
                nullptr,
                [](const ReactionTask&) { return 0; },
                [](const ReactionTask&) { return util::ThreadPoolDescriptor{}; },
                [](const ReactionTask&) { return util::GroupDescriptor{}; });

            task->callback = [this, tasks = std::move(tasks)](const ReactionTask& /*task*/) {
                for (auto& idle_task : tasks) {
                    // Submit all the idle tasks to the scheduler
                    scheduler.submit(idle_task->get_task(), false);
                }
            };

            return Task{std::move(task), std::move(idle_lock)};
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
