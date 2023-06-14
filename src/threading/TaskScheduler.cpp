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

#include "TaskScheduler.hpp"

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <system_error>

#include "../dsl/word/MainThread.hpp"
#include "../util/update_current_thread_priority.hpp"

namespace NUClear {
namespace threading {

    bool is_runnable(const std::unique_ptr<ReactionTask>& task, const uint64_t& pool_id) {
        return task->thread_pool_descriptor.pool_id == pool_id;
    }

    TaskScheduler::TaskScheduler() : running(true) {
        pool_map[std::this_thread::get_id()] = util::ThreadPoolIDSource::MAIN_THREAD_POOL_ID;
    }

    void TaskScheduler::create_pool(const util::ThreadPoolDescriptor& pool) {

        // Pool already exists
        if (pools.count(pool.pool_id) > 0 && pools.at(pool.pool_id).thread_count > 0) {
            return;
        }

        // Make a copy of the pool descriptor
        pools.insert({pool.pool_id, util::ThreadPoolDescriptor{pool.pool_id, pool.thread_count}});

        auto p         = pool;
        auto pool_func = [this, p] {
            // Wait at a high (but not realtime) priority to reduce latency
            // for picking up a new task
            update_current_thread_priority(1000);

            while (running.load() || !queue.empty()) {
                auto task = get_task(p.pool_id);

                if (task) {
                    task->run();
                    if (task->keep_alive) {
                        auto new_task = task->parent.get_task();
                        submit(std::move(new_task));
                    }
                }

                // Back up to realtime while waiting
                update_current_thread_priority(1000);
            }
        };

        // Start all our threads
        /* mutex scope */ {
            const std::lock_guard<std::mutex> threads_lock(threads_mutex);
            for (size_t i = 0; i < pool.thread_count; ++i) {
                threads.push_back(std::make_unique<std::thread>(pool_func));
                pool_map[threads.back()->get_id()] = pool.pool_id;
            }
        }
    }

    void TaskScheduler::start(const size_t& thread_count) {

        // Make the default pool
        create_pool(util::ThreadPoolDescriptor{util::ThreadPoolIDSource::DEFAULT_THREAD_POOL_ID, thread_count});

        // Run main thread tasks
        while (running.load()) {
            auto task = get_task(util::ThreadPoolIDSource::MAIN_THREAD_POOL_ID);

            if (task) {
                task->run();
                if (task->keep_alive) {
                    auto new_task = task->parent.get_task();
                    submit(std::move(new_task));
                }
            }
        }

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
        const std::lock_guard<std::mutex> lock(mutex);
        running.store(false);
        condition.notify_all();
    }

    void TaskScheduler::submit(std::unique_ptr<ReactionTask>&& task) {

        // We do not accept new tasks once we are shutdown
        if (running.load()) {

            // Make sure the pool is created
            create_pool(task->thread_pool_descriptor);

            // Check to see if this task was the result of `emit<Direct>`
            if (task->immediate) {
                if (is_runnable(task, pool_map.at(std::this_thread::get_id()))
                    || is_runnable(task, util::ThreadPoolIDSource::DEFAULT_THREAD_POOL_ID)) {
                    task->run();
                    return;
                }
                // Not runnable, stick it in the queue like nothing happened
            }

            /* Mutex Scope */ {
                const std::lock_guard<std::mutex> lock(mutex);

                // Find where to insert the new task to maintain task order
                auto it = std::lower_bound(queue.begin(), queue.end(), task, std::less<>());

                // Insert before the found position
                queue.insert(it, std::forward<std::unique_ptr<ReactionTask>>(task));
            }
        }

        // Notify a thread that it can proceed
        const std::lock_guard<std::mutex> lock(mutex);
        condition.notify_all();
    }

    std::unique_ptr<ReactionTask> TaskScheduler::get_task(const uint64_t& pool_id) {

        while (running.load() || !queue.empty()) {

            std::unique_lock<std::mutex> lock(mutex);

            for (auto it = queue.begin(); it != queue.end(); ++it) {
                // Check if we can run it
                if (is_runnable(*it, pool_id)) {
                    // Move the task out of the queue
                    std::unique_ptr<ReactionTask> task = std::move(*it);

                    // Erase the old position in the queue
                    queue.erase(it);

                    // Return the task
                    return task;
                }
            }

            // Wait for something to happen!
            condition.wait(lock);
        }

        return nullptr;
    }
}  // namespace threading
}  // namespace NUClear
