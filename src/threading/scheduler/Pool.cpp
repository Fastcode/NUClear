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

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "../../dsl/word/MainThread.hpp"
#include "../../dsl/word/Pool.hpp"
#include "../../id.hpp"
#include "../../threading/Reaction.hpp"
#include "../../util/Inline.hpp"
#include "../ReactionTask.hpp"
#include "CountingLock.hpp"
#include "Scheduler.hpp"
#include "queue/MPSCQueue.hpp"
#include "queue/Priority.hpp"
#include "queue/TaskQueue.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        Pool::Pool(Scheduler& scheduler, std::shared_ptr<const util::ThreadPoolDescriptor> descriptor)
            : descriptor(std::move(descriptor)), scheduler(scheduler) {

            // Pools declared with a single worker (e.g. MainThread, the Trace pool, any user pool with
            // `concurrency = 1`) only ever have one consumer; use the lighter MPSC queue for them.
            // Pools where the default-pool concurrency may differ from the descriptor's nominal value
            // are conservatively given the MPMC queue.
            const bool single_consumer =
                this->descriptor->concurrency == 1 && this->descriptor != dsl::word::Pool<>::descriptor();
            for (auto& bucket : buckets) {
                if (single_consumer) {
                    bucket = std::make_unique<queue::MPSCQueue<Task>>();
                }
                else {
                    bucket = std::make_unique<queue::TaskQueue<Task>>();
                }
            }

            if (this->descriptor->counts_for_idle) {
                scheduler.active_pools.fetch_add(1, std::memory_order_relaxed);
                pool_idle = std::make_unique<CountingLock>(scheduler.active_pools);
            }
        }

        Pool::~Pool() {
            stop(Pool::StopType::FORCE);
            join();
            scheduler.active_pools.fetch_sub(descriptor->counts_for_idle ? 1 : 0, std::memory_order_relaxed);
        }

        void Pool::start() {
            const int n_threads = descriptor == dsl::word::Pool<>::descriptor() ? scheduler.default_pool_concurrency
                                                                                : descriptor->concurrency;

            active = descriptor->counts_for_idle ? n_threads : 0;

            if (descriptor == dsl::word::MainThread::descriptor()) {
                run();
            }
            else {
                const std::lock_guard<std::mutex> lock(mutex);
                for (int i = 0; i < n_threads; ++i) {
                    threads.emplace_back(std::make_unique<std::thread>(&Pool::run, this));
                }
            }
        }

        void Pool::stop(const StopType& type) {
            const std::lock_guard<std::mutex> lock(mutex);

            live = true;
            accept.store(descriptor->persistent, std::memory_order_release);

            switch (type) {
                case StopType::NORMAL: {
                    running = descriptor->persistent;
                } break;
                case StopType::FINAL: {
                    running = false;
                } break;
                case StopType::FORCE: {
                    drain_queues();
                    pending_tasks.store(0, std::memory_order_relaxed);
                    running = false;
                } break;
            }
            condition.notify_all();
        }

        void Pool::notify(bool clear_idle) {
            const std::lock_guard<std::mutex> lock(mutex);
            live = true;
            if (clear_idle) {
                pool_idle = nullptr;
            }
            condition.notify_one();
        }

        void Pool::join() const {
            for (const auto& thread : threads) {
                if (thread->joinable()) {
                    thread->join();
                }
            }
        }

        void Pool::submit(Task&& task, bool clear_idle, bool force) {
            if (!force && !accept.load(std::memory_order_acquire)) {
                return;
            }

            const std::size_t bucket = queue::priority_index(task.task->priority);
            buckets[bucket]->enqueue(std::move(task));
            pending_tasks.fetch_add(1, std::memory_order_release);

            const std::lock_guard<std::mutex> lock(mutex);
            if (clear_idle) {
                pool_idle = nullptr;
            }
            live = true;
            condition.notify_one();
        }

        void Pool::register_external_waiter() {
            external_waiters.fetch_add(1, std::memory_order_acq_rel);
        }

        void Pool::unregister_external_waiter() {
            if (external_waiters.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                // Wake any worker that may be parked specifically because external_waiters was > 0.
                const std::lock_guard<std::mutex> lock(mutex);
                condition.notify_all();
            }
        }

        void Pool::add_idle_task(const std::shared_ptr<Reaction>& reaction) {
            const std::lock_guard<std::mutex> lock(mutex);
            idle_tasks.push_back(reaction);

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

        std::shared_ptr<Pool> Pool::current() {
            return current_pool == nullptr ? nullptr : current_pool->shared_from_this();
        }

        bool Pool::is_idle() const {
            const std::lock_guard<std::mutex> lock(mutex);
            return pool_idle != nullptr;
        }

        void Pool::run() {
            Pool::current_pool = this;
            try {
                while (true) {
                    Task task = get_task();
                    task.task->run();
                }
            }
            catch (const ShutdownThreadException&) {
                Pool::current_pool = nullptr;
                return;
            }
        }

        bool Pool::try_dequeue_task(Task& out) {
            for (std::size_t i = 0; i < queue::PRIORITY_BUCKETS; ++i) {
                if (buckets[i]->try_dequeue(out)) {
                    pending_tasks.fetch_sub(1, std::memory_order_release);
                    return true;
                }
            }
            return false;
        }

        void Pool::drain_queues() {
            Task discarded;
            for (auto& bucket : buckets) {
                while (bucket->try_dequeue(discarded)) {
                }
            }
        }

        Pool::Task Pool::get_task() {
            std::unique_lock<std::mutex> lock(mutex);
            while (running || pending_tasks.load(std::memory_order_acquire) > 0
                   || external_waiters.load(std::memory_order_acquire) > 0) {
                bool got = false;
                if (live) {
                    Task task;
                    got = try_dequeue_task(task);
                    if (got) {
                        if (task.lock == nullptr || task.lock->lock()) {
                            thread_idle[std::this_thread::get_id()] = nullptr;
                            pool_idle                               = nullptr;
                            return task;
                        }
                        // The task was dequeued but its lock isn't acquirable. Re-enqueue and
                        // wait for someone to notify us when the lock state changes.
                        const std::size_t bucket = queue::priority_index(task.task->priority);
                        buckets[bucket]->enqueue(std::move(task));
                        pending_tasks.fetch_add(1, std::memory_order_release);
                    }
                }
                live = false;

                // Only account for idle when we genuinely found nothing; threads whose locks
                // fail are not idle, they are blocked waiting for the lock state to change.
                if (!got) {
                    auto idle_task = get_idle_task();
                    if (idle_task.task != nullptr) {
                        return idle_task;
                    }
                }

                condition.wait(lock, [this] {
                    return live
                           || (!running && pending_tasks.load(std::memory_order_acquire) == 0
                               && external_waiters.load(std::memory_order_acquire) == 0);
                });
            }

            condition.notify_all();
            throw ShutdownThreadException();
        }

        Pool::Task Pool::get_idle_task() {
            if (!running || !descriptor->counts_for_idle) {
                return Task{};
            }

            std::vector<std::shared_ptr<Reaction>> tasks;

            auto& local_lock = thread_idle[std::this_thread::get_id()];

            if (local_lock == nullptr) {
                local_lock = std::make_unique<CountingLock>(active);
                if (local_lock->lock()) {
                    tasks.insert(tasks.end(), idle_tasks.begin(), idle_tasks.end());
                }
            }

            if (pool_idle == nullptr && active.load(std::memory_order_relaxed) == 0) {
                pool_idle = std::make_unique<CountingLock>(scheduler.active_pools);

                if (pool_idle->lock()) {
                    const std::lock_guard<std::mutex> lock(scheduler.idle_mutex);
                    tasks.insert(tasks.end(), scheduler.idle_tasks.begin(), scheduler.idle_tasks.end());
                }
            }

            if (tasks.empty()) {
                return Task{};
            }

            auto task = std::make_unique<ReactionTask>(
                nullptr,
                true,
                [](const ReactionTask&) { return 0; },
                [](const ReactionTask&) { return util::Inline::ALWAYS; },
                [](const ReactionTask&) { return dsl::word::Pool<>::descriptor(); },
                [](const ReactionTask&) { return std::set<std::shared_ptr<const util::GroupDescriptor>>{}; });
            task->callback = [this, t = std::move(tasks)](const ReactionTask& /*task*/) {
                for (const auto& idle_task : t) {
                    scheduler.submit(idle_task->get_task());
                }
            };

            return Task{std::move(task)};
        }

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        thread_local Pool* Pool::current_pool = nullptr;

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
