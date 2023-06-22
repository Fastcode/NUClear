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

    bool TaskScheduler::is_runnable(const std::unique_ptr<ReactionTask>& task) {

        if (groups.count(task->group_descriptor.group_id) == 0) {
            groups[task->group_descriptor.group_id] = 0;
        }

        // Task can run if the group it belongs to has spare threads
        if (groups.at(task->group_descriptor.group_id) < task->group_descriptor.thread_count) {
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
                const std::lock_guard<std::mutex> group_lock(group_mutex);
                groups.at(task->group_descriptor.group_id)--;
            }
        }
    }

    void TaskScheduler::pool_func(std::shared_ptr<PoolQueue> pool) {

        // Set the thread pool for this thread so it can be accessed elsewhere
        current_queue = &pool;

        // When task is nullptr there are no more tasks to get and the scheduler is shutting down
        for (auto task = get_task(); task != nullptr; task = get_task()) {
            // Run the current task
            run_task(std::move(task));
        }

        // Clear the current queue so it can eventually be deleted
        current_queue = nullptr;
    }

    TaskScheduler::TaskScheduler() {
        // Make the queue for the main thread
        pool_queues[util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID] =
            std::make_shared<PoolQueue>(util::ThreadPoolDescriptor{util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID, 1});
    }

    void TaskScheduler::start_threads(const std::shared_ptr<PoolQueue>& pool) {
        // The main thread never needs to be started
        if (pool->pool_descriptor.pool_id != util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID) {
            std::lock_guard<std::mutex> lock(pool->mutex);
            while (pool->threads.size() < pool->pool_descriptor.thread_count) {
                pool->threads.emplace_back(std::make_unique<std::thread>(&TaskScheduler::pool_func, this, pool));
            }
        }
    }

    void TaskScheduler::create_pool(const util::ThreadPoolDescriptor& pool) {
        // If the pool does not exist, create it
        const std::lock_guard<std::mutex> pool_lock(pool_mutex);
        if (pool_queues.count(pool.pool_id) == 0) {
            // Create the pool
            auto queue                = std::make_shared<PoolQueue>(pool);
            pool_queues[pool.pool_id] = queue;

            // If the scheduler has not yet started then don't start the threads for this pool yet
            if (started.load()) {
                start_threads(queue);
            }
        }
    }

    void TaskScheduler::start(const size_t& thread_count) {

        // Make the default pool
        create_pool(util::ThreadPoolDescriptor{util::ThreadPoolDescriptor::DEFAULT_THREAD_POOL_ID, thread_count});

        // The scheduler is now started
        started.store(true);

        // Start all our threads
        for (const auto& pool : pool_queues) {
            start_threads(pool.second);
        }

        // Run main thread tasks
        pool_func(pool_queues.at(util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID));

        /**
         * Once the main thread reaches this point it is because the powerplant, and by extension the scheduler, have
         * been shutdown and the main thread is now about to leave the scheduler.
         */

        // Poke all of the threads to make sure they are awake and then wait for them to finish
        for (auto& pool : pool_queues) {
            pool.second->condition.notify_all();
            for (auto& thread : pool.second->threads) {
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
    }

    void TaskScheduler::shutdown() {
        started.store(false);
        running.store(false);
        for (auto& pool : pool_queues) {
            pool.second->condition.notify_all();
        }
    }

    void TaskScheduler::submit(std::unique_ptr<ReactionTask>&& task) {

        // Check to see if this task should be run immediately
        // Direct tasks can run after shutdown and before starting, provided they can be run immediately
        if (task->immediate) {
            // Check to see if this task is runnable in the current thread
            // If it isn't we can just queue it up with all of the other non-immediate task
            if (is_runnable(task)) {
                run_task(std::move(task));
                return;
            }
        }

        // We do not accept new tasks once we are shutdown
        if (running.load()) {

            // Get the appropiate pool for this task
            std::shared_ptr<PoolQueue> pool;
            /* mutex scope */ {
                const std::lock_guard<std::mutex> pool_lock(pool_mutex);
                pool = pool_queues.at(task->thread_pool_descriptor.pool_id);
            }

            // Find where to insert the new task to maintain task order
            std::lock_guard<std::mutex> queue_lock(pool->mutex);
            auto& queue = pool->queue;
            auto it     = std::lower_bound(queue.begin(), queue.end(), task);
            queue.insert(it, std::move(task));

            // Notify a single thread that there is a new task
            pool->condition.notify_one();
        }
    }

    std::unique_ptr<ReactionTask> TaskScheduler::get_task() {

        // Wait at a high (but not realtime) priority to reduce latency for picking up a new task
        update_current_thread_priority(1000);

        // Get the queue for this thread from its thread local storage
        std::shared_ptr<PoolQueue> pool = *current_queue;
        auto& queue                     = pool->queue;
        auto& condition                 = pool->condition;

        // Keep looking for tasks while the scheduler is still running, or while there are still tasks to process
        std::unique_lock<std::mutex> lock(pool->mutex);
        while (running.load() || !pool->queue.empty()) {

            // Only one thread can be checking group concurrency at a time otherwise the ordering might not be correct
            /* mutex scope */ {
                std::lock_guard<std::mutex> group_lock(group_mutex);

                // Iterate over all the tasks in the current thread pool queue, looking for one that we can run
                for (auto it = queue.begin(); it != queue.end(); ++it) {

                    // Check if we can run the task
                    if (is_runnable(*it)) {

                        // Move the task out of the queue
                        std::unique_ptr<ReactionTask> task = std::move(*it);

                        // Erase the old position in the queue
                        queue.erase(it);

                        // Return the task
                        return task;
                    }
                }
            }

            // Wait for something to happen!
            condition.wait(lock);
        }

        // No more tasks and scheduler has shutdown
        return nullptr;
    }


    /// @brief a pointer to the pool_queue for the current thread so it does not have to access via the map
    ATTRIBUTE_TLS std::shared_ptr<TaskScheduler::PoolQueue>* TaskScheduler::current_queue = nullptr;

}  // namespace threading
}  // namespace NUClear
