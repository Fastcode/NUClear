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
#include "Scheduler.hpp"

#include <algorithm>

#include "../../dsl/word/MainThread.hpp"
#include "CombinedLock.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        Scheduler::Scheduler(const size_t& thread_count) {
            // Make the default pool with the correct number of threads
            auto default_descriptor         = util::ThreadPoolDescriptor{};
            default_descriptor.thread_count = thread_count;
            get_pool(default_descriptor);
        }

        void Scheduler::start() {
            // We have to scope this mutex, otherwise the main thread will hold the mutex while it is running
            /*mutex scope*/ {
                const std::lock_guard<std::mutex> lock(pools_mutex);

                started = true;
                // Start all of the pools except the main thread pool
                for (auto& pool : pools) {
                    if (pool.first != NUClear::id_t(util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID)) {
                        pool.second->start();
                    }
                }
            }

            // Start the main thread pool which will block the main thread
            get_pool(dsl::word::MainThread::descriptor())->start();

            // The main thread will reach this point when the PowerPlant is shutting down
            // Calling stop on each pool will wait for each pool to finish processing all tasks before returning
            for (auto& pool : pools) {
                pool.second->join();
            }
        }

        void Scheduler::stop() {
            running.store(false, std::memory_order_release);
            const std::lock_guard<std::mutex> lock(pools_mutex);
            for (auto& pool : pools) {
                pool.second->stop();
            }
        }

        void Scheduler::add_idle_task(const util::ThreadPoolDescriptor& desc,
                                      const std::shared_ptr<Reaction>& reaction) {
            // If this is the "ALL" pool then add it to the schedulers list of tasks
            if (desc.pool_id == util::ThreadPoolDescriptor::AllPools().pool_id) {
                /*mutex scope*/ {
                    const std::lock_guard<std::mutex> lock(idle_mutex);
                    idle_tasks.push_back(reaction);
                }
                // Notify the main thread pool just in case there were no global idle tasks and now there are
                get_pool(dsl::word::MainThread::descriptor())->notify();
            }
            else {
                get_pool(desc)->add_idle_task(reaction);
            }
        }

        void Scheduler::remove_idle_task(const util::ThreadPoolDescriptor& desc, const NUClear::id_t& id) {
            // If this is the "ALL" pool then remove it from the schedulers list of tasks
            if (desc.pool_id == util::ThreadPoolDescriptor::AllPools().pool_id) {
                const std::lock_guard<std::mutex> lock(idle_mutex);
                idle_tasks.erase(
                    std::remove_if(idle_tasks.begin(), idle_tasks.end(), [&](const auto& r) { return r->id == id; }),
                    idle_tasks.end());
            }
            else {
                get_pool(desc)->remove_idle_task(id);
            }
        }

        std::shared_ptr<Pool> Scheduler::get_pool(const util::ThreadPoolDescriptor& desc) {
            const std::lock_guard<std::mutex> lock(pools_mutex);
            // If the pool does not exist, create it
            if (pools.count(desc.pool_id) == 0) {
                // Create the pool
                auto pool           = std::make_shared<Pool>(*this, desc);
                pools[desc.pool_id] = pool;

                // Don't start the main thread here, it will be started in the start function
                // If the scheduler has not yet started then don't start the threads for this pool yet
                if (desc.pool_id != util::ThreadPoolDescriptor::MAIN_THREAD_POOL_ID && started) {
                    pool->start();
                }
            }

            return pools.at(desc.pool_id);
        }

        std::shared_ptr<Group> Scheduler::get_group(const util::GroupDescriptor& desc) {
            const std::lock_guard<std::mutex> lock(groups_mutex);
            // If the group does not exist, create it
            if (groups.count(desc.group_id) == 0) {
                groups[desc.group_id] = std::make_shared<Group>(desc);
            }

            return groups.at(desc.group_id);
        }

        std::unique_ptr<Lock> Scheduler::get_groups_lock(const NUClear::id_t task_id,
                                                         const int& priority,
                                                         const std::shared_ptr<Pool>& pool,
                                                         const std::set<util::GroupDescriptor>& descs) {

            // No groups
            if (descs.empty()) {
                return nullptr;
            }

            // Make a lock which waits for all the groups to be unlocked
            auto lock = std::make_unique<CombinedLock>();
            for (auto& desc : descs) {
                lock->add(get_group(desc)->lock(task_id, priority, [pool] { pool->notify(); }));
            }

            return lock;
        }

        void Scheduler::submit(std::unique_ptr<ReactionTask>&& task, const bool& immediate) noexcept {

            // Get the pool and locks for the group group
            auto pool       = get_pool(task->pool_descriptor);
            auto group_lock = get_groups_lock(task->id, task->priority, pool, task->group_descriptors);

            // If this task should run immediately and not limited by the group lock
            if (immediate && (group_lock == nullptr || group_lock->lock())) {
                task->run();
                return;
            }

            // Submit the task to the appropriate pool
            if (running.load(std::memory_order_acquire)) {
                pool->submit({std::move(task), std::move(group_lock)});
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
