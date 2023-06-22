/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2022 Trent Houliston <trent@houliston.me>
 *               2023      Trent Houliston <trent@houliston.me>, Alex Biddulph <bidskii@gmail.com>
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

#include "TaskScheduler.hpp"

#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <system_error>

#include "../dsl/word/MainThread.hpp"
#include "../util/update_current_thread_priority.hpp"

namespace NUClear {
namespace threading {

    bool TaskScheduler::is_runnable(const std::unique_ptr<ReactionTask>& task, const uint64_t& pool_id) {

        // Task can run if it is meant to run on the current thread pool
        // Immediate tasks don't care about the pool, only the group concurrency is relevant
        const bool correct_pool = task->immediate || pool_id == task->thread_pool_descriptor.pool_id;

        // Task can run if the group it belongs to has spare threads
        if (groups.at(task->group_descriptor.group_id) < task->group_descriptor.thread_count && correct_pool) {
            // This task is about to run in this group, increase the number of active tasks in the group
            groups.at(task->group_descriptor.group_id)++;
            return true;
        }

        return false;
    }

    void TaskScheduler::run_task(std::unique_ptr<ReactionTask>&& task) {
        if (task) {
            task->run();

            // This task is no longer running, decrease the number of active tasks in the group
            /* mutex scope */ {
                const std::lock_guard<std::mutex> group_lock(queue_mutex);
                groups.at(task->group_descriptor.group_id)--;
            }
        }
    }

    void TaskScheduler::pool_func(const util::ThreadPoolDescriptor& pool) {
        while (running.load() || !queue.at(pool.pool_id).empty()) {
            // Wait at a high (but not realtime) priority to reduce latency
            // for picking up a new task
            update_current_thread_priority(1000);

            run_task(get_task(pool.pool_id));
        }
    }

    TaskScheduler::TaskScheduler() {
        // Setup everything (thread pool, task queue, mutex, condition variable) for the main thread here
        pools[util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID] =
            util::ThreadPoolDescriptor{util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID, 1};
        pool_map[std::this_thread::get_id()] = util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID;
        queue.emplace(util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID, std::vector<std::unique_ptr<ReactionTask>>{});
    }

    void TaskScheduler::start_threads(const util::ThreadPoolDescriptor& pool) {
        // The main thread never needs to be started
        if (pool.pool_id != util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID) {
            const std::lock_guard<std::mutex> pool_lock(pool_mutex);
            for (size_t i = 0; i < pool.thread_count; ++i) {
                threads.push_back(std::make_unique<std::thread>(&TaskScheduler::pool_func, this, pool));
                pool_map[threads.back()->get_id()] = pool.pool_id;
            }
        }
    }

    void TaskScheduler::create_pool(const util::ThreadPoolDescriptor& pool) {
        // Pool already exists
        /* mutex scope */ {
            const std::lock_guard<std::mutex> pool_lock(pool_mutex);
            if (pools.count(pool.pool_id) > 0 && pools.at(pool.pool_id).thread_count > 0) {
                return;
            }

            // Make a copy of the pool descriptor
            pools[pool.pool_id] = util::ThreadPoolDescriptor{pool.pool_id, pool.thread_count};
        }

        // Make sure the task queue is created for this pool
        /* mutex scope */ {
            const std::lock_guard<std::mutex> queue_lock(queue_mutex);
            if (queue.count(pool.pool_id) == 0) {
                queue.emplace(pool.pool_id, std::vector<std::unique_ptr<ReactionTask>>{});
            }
        }

        // If the scheduler has not yet started then don't start the threads for this pool yet
        if (started.load()) {
            start_threads(pool);
        }
    }

    void TaskScheduler::start(const size_t& thread_count) {

        // Make the default pool
        create_pool(util::ThreadPoolDescriptor{util::ThreadPoolDescriptor::DEFAULT_THREAD_POOL_ID, thread_count});

        // The scheduler is now started
        started.store(true);

        // Start all our threads
        for (const auto& pool : pools) {
            start_threads(pool.second);
        }

        // Run main thread tasks
        pool_func(pools.at(util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID));

        /**
         * Once the main thread reaches this point it is because the powerplant, and by extension the schedduler, have
         * been shutdown and the main thread is now about to leave the scheduler.
         */

        // Poke all of the threads to make sure they are awake
        queue_condition.notify_all();

        // Now wait for all the threads to finish executing
        for (auto& thread : threads) {
            try {
                if (thread->joinable()) {
                    thread->join();
                }
            }
            // This gets thrown some time if between checking if joinable and joining
            // the thread is no longer joinable
            catch (const std::system_error&) {
            }
        }
    }

    void TaskScheduler::shutdown() {
        started.store(false);
        running.store(false);
        queue_condition.notify_all();
    }

    void TaskScheduler::submit(std::unique_ptr<ReactionTask>&& task) {

        // Ignore nullptrs, take this as an opportunity to wake a thread up
        if (task == nullptr) {
            queue_condition.notify_all();
            return;
        }

        // Extract the thread pool descriptor from the current task
        const util::ThreadPoolDescriptor current_pool = task->thread_pool_descriptor;

        // Make sure the pool is created
        create_pool(current_pool);

        // Make sure we know about this group
        /* mutex scope */ {
            const std::lock_guard<std::mutex> group_lock(queue_mutex);
            const uint64_t group_id = task->group_descriptor.group_id;
            if (groups.count(group_id) == 0) {
                groups.emplace(group_id, 0);
            }
        }

        // Check to see if this task was the result of `emit<Direct>`
        // Direct tasks can run after shutdown and before starting, provided they can be run immediately
        if (task->immediate) {
            // Map the current thread to the thread pool it belongs to
            uint64_t thread_pool = util::ThreadPoolDescriptor::DEFAULT_THREAD_POOL_ID;
            /* mutex scope */ {
                const std::lock_guard<std::mutex> pool_lock(pool_mutex);
                if (pool_map.count(std::this_thread::get_id()) > 0) {
                    thread_pool = pool_map.at(std::this_thread::get_id());
                }
            }

            // Check to see if this task is runnable in the current thread
            // If it isn't we can just queue it up with all of the other non-immediate task
            if (is_runnable(task, thread_pool)) {
                run_task(std::move(task));
                return;
            }
        }

        // We do not accept new tasks once we are shutdown
        if (running.load()) {
            const std::lock_guard<std::mutex> queue_lock(queue_mutex);

            // Find where to insert the new task to maintain task order
            auto it =
                std::lower_bound(queue.at(current_pool.pool_id).begin(), queue.at(current_pool.pool_id).end(), task);

            // Insert before the found position
            queue.at(current_pool.pool_id).insert(it, std::move(task));
        }

        // Notify all threads that there is a new task to be processed
        queue_condition.notify_all();
    }

    std::unique_ptr<ReactionTask> TaskScheduler::get_task(const uint64_t& pool_id) {

        std::unique_lock<std::mutex> queue_lock(queue_mutex);

        // Keep looking for tasks while the scheduler is still running, or while there are still tasks to process
        while (running.load() || !queue[pool_id].empty()) {

            // Iterate over all the tasks in the current thread pool queue, looking for one that we can run
            for (auto it = queue.at(pool_id).begin(); it != queue.at(pool_id).end(); ++it) {

                // Check if we can run the task
                if (is_runnable(*it, pool_id)) {

                    // Move the task out of the queue
                    std::unique_ptr<ReactionTask> task = std::move(*it);

                    // Erase the old position in the queue
                    queue.at(pool_id).erase(it);

                    // Return the task
                    return task;
                }
            }

            // Wait for something to happen!
            queue_condition.wait(queue_lock);
        }

        // Any thread that passes through here is doing so because the scheduler is shutting down
        // Send a notification to the other threads to make sure they aren't sleeping on the job
        queue_lock.unlock();
        queue_condition.notify_all();

        // No more tasks and scheduler has shutdown
        return nullptr;
    }
}  // namespace threading
}  // namespace NUClear
