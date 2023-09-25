/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
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

#include "TaskScheduler.hpp"

#include <algorithm>
#include <atomic>
#include <iostream>
#include <map>
#include <mutex>
#include <system_error>

#include "../dsl/word/MainThread.hpp"
#include "../util/update_current_thread_priority.hpp"

namespace NUClear {
namespace threading {

    bool TaskScheduler::is_runnable(const util::GroupDescriptor& group) {

        // Default group is always runnable
        if (group.group_id == 0) {
            return true;
        }

        if (groups.count(group.group_id) == 0) {
            groups[group.group_id] = 0;
        }

        // Task can run if the group it belongs to has spare threads
        if (groups.at(group.group_id) < group.thread_count) {
            // This task is about to run in this group, increase the number of active tasks in the group
            groups.at(group.group_id)++;
            return true;
        }

        return false;
    }

    void TaskScheduler::run_task(Task&& task) {
        task.run();

        // We need to do group counting if this isn't the default group
        if (task.group_descriptor.group_id != 0) {
            const std::lock_guard<std::mutex> group_lock(group_mutex);
            --groups.at(task.group_descriptor.group_id);
        }
    }

    void TaskScheduler::pool_func(std::shared_ptr<PoolQueue> pool) {

        // Set the thread pool for this thread so it can be accessed elsewhere
        current_queue = &pool;

        // When task is nullptr there are no more tasks to get and the scheduler is shutting down
        while (running.load() || !pool->queue.empty()) {
            try {
                // Run the next task
                run_task(get_task());
            }
            catch (...) {
            }
        }

        // Clear the current queue
        current_queue = nullptr;
    }

    TaskScheduler::TaskScheduler(const size_t& thread_count) {
        // Make the queue for the main thread
        pool_queues[util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID] =
            std::make_shared<PoolQueue>(util::ThreadPoolDescriptor{util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID, 1});

        // Make the default pool with the correct number of threads
        get_pool_queue(util::ThreadPoolDescriptor{util::ThreadPoolDescriptor::DEFAULT_THREAD_POOL_ID, thread_count});
    }

    void TaskScheduler::start_threads(const std::shared_ptr<PoolQueue>& pool) {
        // The main thread never needs to be started
        if (pool->pool_descriptor.pool_id != util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID) {
            const std::lock_guard<std::mutex> lock(pool->mutex);
            while (pool->threads.size() < pool->pool_descriptor.thread_count) {
                pool->threads.emplace_back(std::make_unique<std::thread>(&TaskScheduler::pool_func, this, pool));
            }
        }
    }

    std::shared_ptr<TaskScheduler::PoolQueue> TaskScheduler::get_pool_queue(const util::ThreadPoolDescriptor& pool) {
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

        return pool_queues.at(pool.pool_id);
    }

    void TaskScheduler::start() {

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
            /* mutex scope */ {
                const std::lock_guard<std::mutex> queue_lock(pool.second->mutex);
                pool.second->condition.notify_all();
            }
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
            const std::lock_guard<std::mutex> lock(pool.second->mutex);
            pool.second->condition.notify_all();
        }
    }

    void TaskScheduler::submit(const NUClear::id_t& id,
                               const int& priority,
                               const util::GroupDescriptor& group_descriptor,
                               const util::ThreadPoolDescriptor& pool_descriptor,
                               const bool& immediate,
                               std::function<void()>&& func) {

        // Move the arguments into a struct
        Task task{id, priority, group_descriptor, pool_descriptor, std::move(func)};

        // Immediate tasks are executed directly on the current thread if they can be
        // If something is blocking them from running right now they are added to the queue
        if (immediate) {
            bool runnable = false;
            /* mutex scope */ {
                const std::lock_guard<std::mutex> group_lock(group_mutex);
                runnable = is_runnable(task.group_descriptor);
            }
            if (runnable) {
                run_task(std::move(task));
                return;
            }
        }

        // We do not accept new tasks once we are shutdown
        if (running.load()) {
            // Get the appropiate pool for this task
            const std::shared_ptr<PoolQueue> pool = get_pool_queue(task.thread_pool_descriptor);

            // Find where to insert the new task to maintain task order
            const std::lock_guard<std::mutex> queue_lock(pool->mutex);
            auto& queue = pool->queue;
            auto it     = std::lower_bound(queue.begin(), queue.end(), task);
            queue.insert(it, std::move(task));

            // Notify a single thread that there is a new task
            pool->condition.notify_one();
        }
    }

    TaskScheduler::Task TaskScheduler::get_task() {

        // Wait at a high (but not realtime) priority to reduce latency for picking up a new task
        update_current_thread_priority(1000);

        if (current_queue == nullptr) {
            throw std::runtime_error("Only threads managed by the TaskScheduler can get tasks");
        }

        // Get the queue for this thread from its thread local storage
        const std::shared_ptr<PoolQueue> pool = *current_queue;
        auto& queue                           = pool->queue;
        auto& condition                       = pool->condition;

        // Keep looking for tasks while the scheduler is still running, or while there are still tasks to process
        std::unique_lock<std::mutex> lock(pool->mutex);
        while (running.load() || !queue.empty()) {

            // Only one thread can be checking group concurrency at a time otherwise the ordering might not be correct
            /* mutex scope */ {
                const std::lock_guard<std::mutex> group_lock(group_mutex);

                // Iterate over all the tasks in the current thread pool queue, looking for one that we can run
                for (auto it = queue.begin(); it != queue.end(); ++it) {

                    // Check if we can run the task
                    if (is_runnable(it->group_descriptor)) {

                        // Move the task out of the queue
                        Task task = std::move(*it);

                        // Erase the old position in the queue
                        queue.erase(it);

                        // Return the task
                        return task;
                    }
                }

                // If pool concurrency is greater than group concurrency some threads can be left with nothing to do.
                // Since running is false there will likely never be anything new to do and we are shutting down anyway.
                // So if we can't find a task to run we should just quit.
                if (!running.load()) {
                    condition.notify_all();
                    throw ShutdownThreadException();
                }
            }

            // Wait for something to happen!
            // We don't have a condition on this lock as the check would be this doing this loop again to see if there
            // are any tasks we can execute (checking all the groups) so therefore we already did the entry predicate.
            // Putting a condition on this would stop spurious wakeups of which the cost would be equal to the loop.
            condition.wait(lock);  // NOSONAR
        }

        // If we get out here then we are finished running.
        throw ShutdownThreadException();
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    ATTRIBUTE_TLS std::shared_ptr<TaskScheduler::PoolQueue>* TaskScheduler::current_queue = nullptr;

}  // namespace threading
}  // namespace NUClear
