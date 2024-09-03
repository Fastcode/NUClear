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
#include <stdexcept>

#include "../../dsl/word/MainThread.hpp"
#include "../../dsl/word/Pool.hpp"
#include "CombinedLock.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        Scheduler::Scheduler(const int& default_pool_concurrency) : default_pool_concurrency(default_pool_concurrency) {
            // Create the main thread pool and assign it as our "current pool" so things we do pre startup are assigned
            Pool::current_pool = get_pool(dsl::word::MainThread::descriptor()).get();
        }

        void Scheduler::start() {
            // We have to scope this mutex, otherwise the main thread will hold the mutex while it is running
            /*mutex scope*/ {
                const std::lock_guard<std::mutex> lock(pools_mutex);

                started = true;
                // Start all of the pools except the main thread pool
                for (const auto& pool : pools) {
                    if (pool.first != dsl::word::MainThread::descriptor()) {
                        pool.second->start();
                    }
                }
            }

            // Start the main thread pool which will block the main thread
            get_pool(dsl::word::MainThread::descriptor())->start();

            // The main thread will reach this point when the PowerPlant is shutting down
            // Sort the pools so that the pools that ignore shutdown are last to be forced to stop
            std::vector<std::shared_ptr<Pool>> pools_to_stop;
            pools_to_stop.reserve(pools.size());
            for (const auto& pool : pools) {
                pools_to_stop.push_back(pool.second);
            }
            std::sort(pools_to_stop.begin(), pools_to_stop.end(), [](const auto& lhs, const auto& rhs) {
                const bool& a = lhs->descriptor->persistent;
                const bool& b = rhs->descriptor->persistent;
                return !a && b;
            });
            for (const auto& pool : pools_to_stop) {
                // This is the final stop call
                // By this point we have waited for all tasks to finish on the pools that don't ignore shutdown
                // So now we can tell the pools that ignore shutdown to stop
                pool->stop(Pool::StopType::FINAL);
                pool->join();
            }
        }

        void Scheduler::stop(bool force) {
            running.store(false, std::memory_order_release);
            const std::lock_guard<std::mutex> lock(pools_mutex);
            for (const auto& pool : pools) {
                pool.second->stop(force ? Pool::StopType::FORCE : Pool::StopType::NORMAL);
            }
        }

        void Scheduler::add_idle_task(const std::shared_ptr<Reaction>& reaction,
                                      const std::shared_ptr<const util::ThreadPoolDescriptor>& desc) {
            // If this doesn't have a pool specifier it's for all pools
            if (desc == nullptr) {
                /*mutex scope*/ {
                    const std::lock_guard<std::mutex> lock(idle_mutex);
                    idle_tasks.push_back(reaction);
                }
                // Notify the main thread pool just in case there were no global idle tasks and now there are
                // Clear idle status so that these tasks are executed immediately
                get_pool(dsl::word::MainThread::descriptor())->notify(true);
            }
            else {
                get_pool(desc)->add_idle_task(reaction);
            }
        }

        void Scheduler::remove_idle_task(const NUClear::id_t& id,
                                         const std::shared_ptr<const util::ThreadPoolDescriptor>& desc) {
            // If this doesn't have a pool specifier it's for all pools
            if (desc == nullptr) {
                const std::lock_guard<std::mutex> lock(idle_mutex);
                idle_tasks.erase(
                    std::remove_if(idle_tasks.begin(), idle_tasks.end(), [&](const auto& r) { return r->id == id; }),
                    idle_tasks.end());
            }
            else {
                get_pool(desc)->remove_idle_task(id);
            }
        }

        std::shared_ptr<Pool> Scheduler::get_pool(const std::shared_ptr<const util::ThreadPoolDescriptor>& desc) {
            const std::lock_guard<std::mutex> lock(pools_mutex);
            // If the pool does not exist, create it
            if (pools.count(desc) == 0) {
                // Don't make new pools if we are shutting down
                if (!running.load(std::memory_order_acquire)) {
                    throw std::invalid_argument(
                        "Cannot create new pools after the scheduler has started shutting down");
                }

                // Create the pool
                auto pool   = std::make_shared<Pool>(*this, desc);
                pools[desc] = pool;

                // Don't start the main thread here, it will be started in the start function
                // If the scheduler has not yet started then don't start the threads for this pool yet
                if (desc != dsl::word::MainThread::descriptor() && started) {
                    pool->start();
                }
            }

            return pools.at(desc);
        }

        std::shared_ptr<Group> Scheduler::get_group(const std::shared_ptr<const util::GroupDescriptor>& desc) {
            const std::lock_guard<std::mutex> lock(groups_mutex);
            // If the group does not exist, create it
            if (groups.count(desc) == 0) {
                groups[desc] = std::make_shared<Group>(desc);
            }

            return groups.at(desc);
        }

        std::unique_ptr<Lock> Scheduler::get_groups_lock(
            const NUClear::id_t& task_id,
            const int& priority,
            const std::shared_ptr<Pool>& pool,
            const std::set<std::shared_ptr<const util::GroupDescriptor>>& descs) {

            // No groups
            if (descs.empty()) {
                return nullptr;
            }

            // Make a lock which waits for all the groups to be unlocked
            auto lock = std::make_unique<CombinedLock>();
            for (const auto& desc : descs) {
                lock->add(get_group(desc)->lock(task_id, priority, [pool] {
                    const bool current_pool_idle = Pool::current() != nullptr && Pool::current()->is_idle();
                    pool->notify(!current_pool_idle);
                }));
            }

            return lock;
        }

        void Scheduler::submit(std::unique_ptr<ReactionTask>&& task) noexcept {
            // Ignore null tasks
            if (task == nullptr) {
                return;
            }

            // Get the pool and locks for the group group
            auto pool       = get_pool(task->pool_descriptor);
            auto group_lock = get_groups_lock(task->id, task->priority, pool, task->group_descriptors);

            // If this task should run immediately and not limited by the group lock
            if (task->run_inline && (group_lock == nullptr || group_lock->lock())) {
                task->run();
            }
            else {
                // Submit the task to the appropriate pool
                // Clear the idle status only if the current pool is not idle
                // This hands the job of managing global idle tasks to this other pool if we were about to do it
                // That way the other pool can decide if it is idle or not
                const bool current_pool_idle = Pool::current() != nullptr && Pool::current()->is_idle();
                pool->submit({std::move(task), std::move(group_lock)}, !current_pool_idle);
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
