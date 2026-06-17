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
#include "../../util/ThreadPriority.hpp"
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
            single_consumer = this->descriptor->concurrency == 1 && this->descriptor != dsl::word::Pool<>::descriptor();
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
            // Force stop the pool threads and wait for them to finish
            try {
                stop(Pool::StopType::FORCE);
            }
            catch (...) {  // NOLINT(bugprone-empty-catch)
                // Draining queued tasks during forced shutdown can throw if a Task's lock
                // destructors re-enter the pool; swallow here rather than std::terminate.
            }
            try {
                join();
            }
            catch (...) {  // NOLINT(bugprone-empty-catch)
                // std::thread::join() may throw std::system_error on failure.
            }
            // One less active pool
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
            // Drained tasks may hold group locks whose destructors can re-enter the pool; defer
            // their destruction until after the mutex is released.
            std::vector<Task> drained;
            {
                std::unique_lock<std::mutex> lock(mutex);

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
                        // A force stop is terminal even for persistent pools: stop accepting new work so
                        // nothing can repopulate the queues after we drain them and wind the threads down.
                        accept.store(false, std::memory_order_release);
                        running = false;

                        // MPSC buckets permit only one consumer. A cross-thread FORCE stop (e.g.
                        // PowerPlant::shutdown(true) from TestBase's timeout thread against a
                        // MainThread or concurrency-1 pool) must delegate queue draining to that
                        // worker instead of calling try_dequeue here.
                        const bool mpsc_consumer_alive =
                            single_consumer && consumer_thread_id != std::thread::id{};
                        const bool on_mpsc_consumer =
                            mpsc_consumer_alive && std::this_thread::get_id() == consumer_thread_id;

                        if (mpsc_consumer_alive && !on_mpsc_consumer) {
                            discard_queues_requested.store(true, std::memory_order_release);
                            condition.notify_all();
                            condition.wait(lock, [this] {
                                return !discard_queues_requested.load(std::memory_order_acquire);
                            });
                            pending_tasks.store(0, std::memory_order_relaxed);
                        }
                        else {
                            drain_queues(drained);
                            pending_tasks.store(0, std::memory_order_relaxed);
                        }
                    } break;
                }
                condition.notify_all();
            }
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
            pending_tasks.fetch_add(1, std::memory_order_release);
            buckets[bucket]->enqueue(std::move(task));

            const std::lock_guard<std::mutex> lock(mutex);
            if (clear_idle) {
                pool_idle = nullptr;
            }
            live = true;
            condition.notify_one();
        }

        ExternalWaiterRegistration::ExternalWaiterRegistration(ExternalWaiterRegistration&& other) noexcept
            : pool_(other.pool_) {
            other.pool_ = nullptr;
        }

        ExternalWaiterRegistration& ExternalWaiterRegistration::operator=(ExternalWaiterRegistration&& other) noexcept {
            if (this != &other) {
                reset();
                pool_       = other.pool_;
                other.pool_ = nullptr;
            }
            return *this;
        }

        ExternalWaiterRegistration::~ExternalWaiterRegistration() {
            reset();
        }

        void ExternalWaiterRegistration::reset() noexcept {
            if (pool_ != nullptr) {
                pool_->unregister_external_waiter();
                pool_ = nullptr;
            }
        }

        ExternalWaiterRegistration Pool::register_external_waiter() {
            external_waiters.fetch_add(1, std::memory_order_acq_rel);

            // Fast exit when no idle reaction could ever fire on this pool. This is the common
            // case on a hot Sync-contended chain (the tasks being parked are real work, not idle
            // triggers), and it keeps this path free of any extra synchronisation: just the
            // external_waiters increment above plus the relaxed loads inside idle_relevant().
            if (!idle_relevant()) {
                return ExternalWaiterRegistration{this};
            }

            // Latch a "should fire idle on next poll" signal. This guarantees the destination
            // pool observes one idle epoch per parked waiter even if the worker is preempted
            // long enough that, by the time it resumes, the drained task is already sitting in
            // the queue (in which case it would otherwise be picked up directly with no idle
            // fire). See Pool::get_task for the consumer.
            //
            // Only acquire the mutex + notify the worker on the 0->1 transition of the latch.
            // Subsequent parkings while the latch is already set don't need to wake the worker
            // again -- the latch already says "fire idle before the next dispatch", and one
            // wake is enough to bring the worker out of condition.wait.
            if (!pending_idle.exchange(true, std::memory_order_acq_rel)) {
                const std::lock_guard<std::mutex> lock(mutex);
                condition.notify_one();
            }
            return ExternalWaiterRegistration{this};
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
            idle_task_count.fetch_add(1, std::memory_order_release);

            if (idle_tasks.size() == 1) {
                condition.notify_one();
            }
        }

        void Pool::remove_idle_task(const NUClear::id_t& id) {
            const std::lock_guard<std::mutex> lock(mutex);
            const auto before = idle_tasks.size();
            idle_tasks.erase(
                std::remove_if(idle_tasks.begin(), idle_tasks.end(), [&](const auto& r) { return r->id == id; }),
                idle_tasks.end());
            idle_task_count.fetch_sub(before - idle_tasks.size(), std::memory_order_release);
        }

        bool Pool::idle_relevant() const {
            return idle_task_count.load(std::memory_order_acquire) > 0
                   || scheduler.global_idle_count.load(std::memory_order_acquire) > 0;
        }

        std::shared_ptr<Pool> Pool::current() {
            return current_pool == nullptr ? nullptr : current_pool->shared_from_this();
        }

        bool Pool::is_idle() const {
            const std::lock_guard<std::mutex> lock(mutex);
            return pool_idle != nullptr;
        }

        void Pool::run() {
            consumer_thread_id = std::this_thread::get_id();
            Pool::current_pool = this;
            try {
                // Set the thread priority to highest while getting tasks
                // This means that this thread will be a FIFO queued task on linux so it won't timeslice
                const util::ThreadPriority priority_lock(PriorityLevel::HIGHEST);
                while (true) {
                    Task task = get_task();
                    task.task->run();
                }
            }
            catch (const ShutdownThreadException&) {
                Pool::current_pool = nullptr;
                consumer_thread_id = std::thread::id{};
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

        void Pool::drain_queues(std::vector<Task>& out) const {
            Task task;
            for (const auto& bucket : buckets) {
                while (bucket->try_dequeue(task)) {
                    out.push_back(std::move(task));
                }
            }
        }

        Pool::Task Pool::get_task() {
            std::unique_lock<std::mutex> lock(mutex);
            while (running || pending_tasks.load(std::memory_order_acquire) > 0
                   || external_waiters.load(std::memory_order_acquire) > 0
                   || discard_queues_requested.load(std::memory_order_acquire)) {
                if (discard_queues_requested.load(std::memory_order_acquire)) {
                    std::vector<Task> discarded;
                    drain_queues(discarded);
                    pending_tasks.store(0, std::memory_order_relaxed);
                    discard_queues_requested.store(false, std::memory_order_release);
                    condition.notify_all();
                    lock.unlock();
                    discarded.clear();
                    lock.lock();
                    continue;
                }

                // If a waiter was parked for this pool since the last time this worker looked,
                // ensure we fire one idle epoch before dispatching the next task. This is the
                // counterpart of the OLD scheduler behaviour where a parked task with a failing
                // group lock sat in the pool queue and forced the worker to poll-fail-and-fall-
                // through to get_idle_task; in the fast path the task is parked in the Group's
                // wait_buckets instead, so without this latch the worker can be preempted long
                // enough for the drained (lock-OK) task to arrive in the queue before the worker
                // polls and end up running it directly, swallowing the idle fire.
                //
                // get_idle_task() is a no-op when this thread is already idle (local_lock set),
                // so a wasted consume here is harmless: the worker just falls through to the
                // normal dequeue path below.
                //
                // The relaxed load short-circuits the (more expensive) read-modify-write on the
                // common path where nothing has been latched, so a busy worker never pays for the
                // exclusive cacheline acquire that exchange() would force every iteration.
                if (pending_idle.load(std::memory_order_acquire)
                    && pending_idle.exchange(false, std::memory_order_acq_rel)) {
                    auto idle_task = get_idle_task();
                    if (idle_task.task != nullptr) {
                        return idle_task;
                    }
                }

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
                        pending_tasks.fetch_add(1, std::memory_order_release);
                        buckets[bucket]->enqueue(std::move(task));
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
                    return live || pending_idle.load(std::memory_order_acquire)
                           || discard_queues_requested.load(std::memory_order_acquire)
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
                [](const ReactionTask&) { return PriorityLevel::HIGHEST; },
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
