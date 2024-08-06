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
#include <map>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "../dsl/word/MainThread.hpp"
#include "../id.hpp"
#include "../util/GroupDescriptor.hpp"
#include "../util/ThreadPoolDescriptor.hpp"
#include "../util/platform.hpp"
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

    void TaskScheduler::run_task(std::unique_ptr<ReactionTask>&& task) {
        task->run();

        // We need to do group counting if this isn't the default group
        if (task->group_descriptor.group_id != 0) {
            const std::lock_guard<std::mutex> group_lock(group_mutex);
            --groups.at(task->group_descriptor.group_id);
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
            if (pool->pool_descriptor.counts_for_idle) {
                --global_runnable_tasks;
                --pool->runnable_tasks;
            }
        }

        // Clear the current queue
        current_queue = nullptr;
    }

    TaskScheduler::TaskScheduler(const size_t& thread_count) {
        // Make the queue for the main thread
        auto main_descriptor                 = dsl::word::MainThread::main_descriptor();
        pool_queues[main_descriptor.pool_id] = std::make_shared<PoolQueue>(main_descriptor);

        // Make the default pool with the correct number of threads
        auto default_descriptor         = util::ThreadPoolDescriptor{};
        default_descriptor.thread_count = thread_count;
        get_pool_queue(default_descriptor);
    }

    void TaskScheduler::start_threads(const std::shared_ptr<PoolQueue>& pool) {
        // The main thread never needs to be started
        if (pool->pool_descriptor.pool_id != NUClear::id_t(util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID)) {
            const std::lock_guard<std::recursive_mutex> lock(pool->mutex);
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
        pool_func(pool_queues.at(NUClear::id_t(util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID)));

        /**
         * Once the main thread reaches this point it is because the powerplant, and by extension the scheduler, have
         * been shutdown and the main thread is now about to leave the scheduler.
         */

        // Poke all of the threads to make sure they are awake and then wait for them to finish
        for (auto& pool : pool_queues) {
            /* mutex scope */ {
                const std::lock_guard<std::recursive_mutex> queue_lock(pool.second->mutex);
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
            const std::lock_guard<std::recursive_mutex> lock(pool.second->mutex);
            pool.second->condition.notify_all();
        }
    }

    void TaskScheduler::PoolQueue::submit(std::unique_ptr<ReactionTask>&& task) {
        const std::lock_guard<std::recursive_mutex> lock(mutex);

        // Insert in sorted order
        PoolQueue::Task item{std::move(task), false};
        queue.insert(std::lower_bound(queue.begin(), queue.end(), item), std::move(item));

        // Notify a single thread that there is a new task
        condition.notify_one();
    }

    void TaskScheduler::submit(std::unique_ptr<ReactionTask>&& task, const bool& immediate) noexcept {

        // Immediate tasks are executed directly on the current thread if they can be
        // If something is blocking them from running right now they are added to the queue
        if (immediate) {
            bool runnable = false;
            /* mutex scope */ {
                const std::lock_guard<std::mutex> group_lock(group_mutex);
                runnable = is_runnable(task->group_descriptor);
            }
            if (runnable) {
                run_task(std::move(task));
                return;
            }
        }

        // We only accept new tasks while the scheduler is running
        if (running.load()) {
            const std::shared_ptr<PoolQueue> pool = get_pool_queue(task->thread_pool_descriptor);
            if (pool->pool_descriptor.counts_for_idle) {
                ++global_runnable_tasks;
                ++pool->runnable_tasks;
            }
            // Get the appropiate pool for this task and submit
            pool->submit(std::move(task));
        }
    }

    void TaskScheduler::add_idle_task(const NUClear::id_t& id,
                                      const util::ThreadPoolDescriptor& pool_descriptor,
                                      std::function<void()>&& task) {

        if (pool_descriptor.pool_id == NUClear::id_t(-1)) {
            const std::lock_guard<std::mutex> lock(pool_mutex);
            idle_tasks.emplace(id, std::move(task));
        }
        else {
            // Get the appropiate pool for this task
            const std::shared_ptr<PoolQueue> pool = get_pool_queue(pool_descriptor);

            // Find where to insert the new task to maintain task order
            const std::lock_guard<std::recursive_mutex> lock(pool->mutex);
            pool->idle_tasks.emplace(id, std::move(task));
        }
    }

    void TaskScheduler::remove_idle_task(const NUClear::id_t& id, const util::ThreadPoolDescriptor& pool_descriptor) {

        if (pool_descriptor.pool_id == NUClear::id_t(-1)) {
            const std::lock_guard<std::mutex> lock(pool_mutex);
            if (idle_tasks.count(id) > 0) {
                idle_tasks.erase(id);
            }
        }
        else {
            // Get the appropiate pool for this task
            const std::shared_ptr<PoolQueue> pool = get_pool_queue(pool_descriptor);

            // Find the idle task with the given id and remove it if it exists
            const std::lock_guard<std::recursive_mutex> lock(pool->mutex);
            if (pool->idle_tasks.count(id) > 0) {
                pool->idle_tasks.erase(id);
            }
        }
    }

    std::unique_ptr<ReactionTask> TaskScheduler::get_task() {

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
        std::unique_lock<std::recursive_mutex> lock(pool->mutex);
        bool idle = false;
        while (running.load() || !queue.empty()) {

            // Only one thread can be checking group concurrency at a time otherwise the ordering might not be correct
            /* mutex scope */ {
                const std::lock_guard<std::mutex> group_lock(group_mutex);

                // Iterate over all the tasks in the current thread pool queue, looking for one that we can run
                for (auto it = queue.begin(); it != queue.end(); ++it) {

                    // Check if we can run the task
                    if (is_runnable(it->task->group_descriptor)) {
                        // Move the task out of the queue
                        auto task = std::move(*it);
                        // Erase the old position in the queue
                        queue.erase(it);

                        // If it was blocked and is now unblocked, there is an additional runnable task
                        if (pool->pool_descriptor.counts_for_idle && task.blocked) {
                            ++global_runnable_tasks;
                            ++pool->runnable_tasks;
                        }

                        // Return the task
                        return std::move(task.task);
                    }

                    // The task is blocked, mark it as such and lower the number of runnable tasks
                    if (!it->blocked) {
                        if (pool->pool_descriptor.counts_for_idle) {
                            --global_runnable_tasks;
                            --pool->runnable_tasks;
                        }
                        it->blocked = true;
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

            if (pool->pool_descriptor.counts_for_idle && !idle) {
                idle = true;

                /* mutex scope */ {
                    const std::lock_guard<std::mutex> idle_lock(idle_mutex);
                    // Local idle tasks
                    if (pool->runnable_tasks == 0) {
                        for (auto& t : pool->idle_tasks) {
                            t.second();
                        }
                    }

                    // Global idle tasks
                    if (global_runnable_tasks == 0) {
                        for (auto& t : idle_tasks) {
                            t.second();
                        }
                    }
                }
            }
            else {

                // Wait for something to happen!
                // We don't have a condition on this lock as the check would be this doing this loop again to see if
                // there are any tasks we can execute (checking all the groups) so therefore we already did the entry
                // predicate. Putting a condition on this would stop spurious wakeups of which the cost would be equal
                // to the loop.
                condition.wait(lock);  // NOSONAR
            }
        }

        // If we get out here then we are finished running.
        throw ShutdownThreadException();
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    ATTRIBUTE_TLS std::shared_ptr<TaskScheduler::PoolQueue>* TaskScheduler::current_queue = nullptr;

}  // namespace threading
}  // namespace NUClear
