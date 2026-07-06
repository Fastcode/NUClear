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
#ifndef NUCLEAR_THREADING_SCHEDULER_POOL_HPP
#define NUCLEAR_THREADING_SCHEDULER_POOL_HPP

#include <array>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../../util/ThreadPoolDescriptor.hpp"
#include "../ReactionTask.hpp"
#include "Lock.hpp"
#include "queue/MPSCQueue.hpp"
#include "queue/Priority.hpp"
#include "queue/Queue.hpp"
#include "queue/TaskQueue.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        // Forward declare the scheduler
        class Scheduler;

        /**
         * RAII registration that keeps a pool's workers alive while a task is parked outside it.
         *
         * Move-only; unregisters on destruction. Obtained from Pool::register_external_waiter().
         */
        class ExternalWaiterRegistration {
        public:
            ExternalWaiterRegistration() noexcept = default;
            ExternalWaiterRegistration(ExternalWaiterRegistration&& other) noexcept;
            ExternalWaiterRegistration& operator=(ExternalWaiterRegistration&& other) noexcept;
            ~ExternalWaiterRegistration();

            ExternalWaiterRegistration(const ExternalWaiterRegistration&)            = delete;
            ExternalWaiterRegistration& operator=(const ExternalWaiterRegistration&) = delete;

        private:
            friend class Pool;
            explicit ExternalWaiterRegistration(Pool* pool) noexcept : pool_(pool) {}
            void reset() noexcept;

            Pool* pool_{nullptr};
        };

        class Pool : public std::enable_shared_from_this<Pool> {
        public:
            enum class StopType : uint8_t {
                /// Normal stop, wait for all tasks to finish and accept no more tasks
                /// Pools which ignore shutdown will continue to accept tasks
                NORMAL,
                /// Final stop request, pools which ignore shutdown will finish when all tasks are done
                /// However they will continue to accept tasks
                FINAL,
                /// Force stop, the queue will be cleared and all threads will be woken
                FORCE
            };

            struct Task {
                /**
                 * @brief Construct a new Task object
                 *
                 * @param task the task to execute
                 * @param lock the RAII lock to hold while the task is being executed
                 */
                Task(std::unique_ptr<ReactionTask>&& task = nullptr, std::unique_ptr<Lock>&& lock = nullptr)
                    : task(std::move(task)), lock(std::move(lock)) {}

                /// Holds the task to execute
                std::unique_ptr<ReactionTask> task;
                /// A lock that is held while the task is being executed.
                /// This lock should release via RAII when the task is done.
                std::unique_ptr<Lock> lock;
            };

            /**
             * Construct a new thread pool with the given descriptor
             *
             * @param scheduler  the scheduler parent of this pool
             * @param descriptor the descriptor for this thread pool
             */
            explicit Pool(Scheduler& scheduler, std::shared_ptr<const util::ThreadPoolDescriptor> descriptor);

            // No moving or copying
            Pool(const Pool&)            = delete;
            Pool(Pool&&)                 = delete;
            Pool& operator=(const Pool&) = delete;
            Pool& operator=(Pool&&)      = delete;

            /**
             * Destroy the Pool object
             *
             * Will stop the pool if it is still running and wait for all threads to exit.
             */
            ~Pool();

            /**
             * Starts the thread pool and begins executing tasks.
             *
             * If the main thread pool is started then the main thread will stay in this function executing tasks until
             * the scheduler is shutdown.
             */
            void start();

            /**
             * Stops the thread pool, all threads are woken and once the task queue is empty the threads will exit.
             * This function returns immediately, use join to wait for the threads to exit.
             *
             * @param type the type of stop to perform
             */
            void stop(const StopType& type);

            /**
             * Notify a thread in this pool that there is work to do.
             *
             * It will wake up a thread if one is waiting for work, otherwise it will be picked up by the next thread.
             *
             * @param clear_idle If true, the idle state of the pool will be cleared
             */
            void notify(bool clear_idle);

            /**
             * Wait for all threads in this pool to exit.
             */
            void join() const;

            /**
             * Submit a new task to this thread pool
             *
             * @param task       The reaction task task to submit
             * @param clear_idle If true, the idle state of the pool will be cleared
             * @param force      If true, submit even if the pool is no longer accepting new tasks
             *                   (used when draining an already in-flight task from elsewhere, e.g. a Group)
             */
            void submit(Task&& task, bool clear_idle, bool force = false);

            /**
             * Register that a task is in flight outside the pool but will eventually be submitted to it.
             *
             * This keeps the pool's workers alive while there are tasks parked in another structure
             * (e.g. a Group's waiter buckets) that point at this pool.
             *
             * @return A move-only handle that unregisters on destruction
             */
            ExternalWaiterRegistration register_external_waiter();

            /**
             * Add an idle task to this pool.
             *
             * This will add a task to the idle task list for this pool.
             *
             * @param reaction the reaction to add to the idle task list
             */
            void add_idle_task(const std::shared_ptr<Reaction>& reaction);

            /**
             * Remove an idle task from this pool.
             *
             * @param id the id of the reaction to remove from the idle task list
             */
            void remove_idle_task(const NUClear::id_t& id);

            /**
             * Returns the thread pool that the current thread is running in, or nullptr if the current thread is not a
             * scheduler thread.
             *
             * @return the thread pool that the current thread is running in
             */
            static std::shared_ptr<Pool> current();

            /**
             * Returns if the current thread is idle.
             *
             * @return true if the current thread is idle
             */
            bool is_idle() const;

            /// The descriptor for this thread pool
            const std::shared_ptr<const util::ThreadPoolDescriptor> descriptor;

        private:
            /**
             * Exception thrown when a thread in the pool should shut down.
             */
            class ShutdownThreadException : public std::exception {};

            /**
             * The main function executed by each thread in the pool.
             *
             * The thread will wait for a task to be available and then execute it.
             * This will continue until the pool is stopped.
             */
            void run();

            /**
             * Get the next task to execute.
             *
             * This will return the next task to execute or block until a task is available.
             *
             * @return the next task to execute
             */
            Task get_task();

            /**
             * Try to dequeue a runnable task from the priority buckets.
             *
             * @param out the task to fill if one is available
             *
             * @return true if a task was dequeued
             */
            bool try_dequeue_task(Task& out);

            /**
             * Drain all tasks from the priority buckets into out.
             *
             * @param out the drained tasks (destruction deferred by the caller)
             */
            void drain_queues(std::vector<Task>& out) const;

            /**
             * Get an idle task to execute or hold.
             *
             * This will return an idle task instance.
             * If the lock on the idle task returns true, it will then execute the idle task which enqueues the tasks
             * that have been declared.
             *
             * If this was not the last thread to become idle, it will return an object which will not lock and the
             * thread should then sleep until it is woken.
             *
             * @return the idle task to execute if it is lockable or hold if it is not
             */
            Task get_idle_task();

            /**
             * Get only this pool's own local idle task (on<Idle<ThisPool>> reactions), without considering the
             * global (all-pools) idle epoch.
             *
             * This exists so the local, per-pool `active` transition can be checked eagerly - as soon as a woken
             * worker notices `pending_idle` - without risking the premature-global-idle bug that firing the
             * global check early can cause (see the comment in get_task() for details). The local `active`
             * counter only ever gates this pool's OWN Idle<ThisPool> reactions, never `scheduler.active_pools`,
             * so firing it eagerly cannot release the global active_pools slot early - it is safe to check as
             * soon as possible, and doing so avoids missing a fleeting active-count-reaches-zero edge that a
             * concurrent task submission could otherwise paper over before the deferred dequeue-first path gets
             * around to checking it.
             *
             * @return the local idle task to execute if it is lockable, or hold if it is not
             */
            Task get_local_idle_task();

            /**
             * Collect this pool's own local idle reactions (on<Idle<ThisPool>>) if this worker is the one that
             * takes the pool's `active` count to zero.
             *
             * Appends the reactions to fire to @p tasks; leaves it untouched if this worker did not win the
             * local idle lock. Shared by both get_local_idle_task() and get_idle_task().
             *
             * @param tasks the accumulator to append any local idle reactions to
             */
            void collect_local_idle_reactions(std::vector<std::shared_ptr<Reaction>>& tasks);

            /**
             * Wrap a collected set of idle reactions in a dispatch task that submits them when run.
             *
             * @param tasks the idle reactions to dispatch (moved from)
             *
             * @return the dispatch task, or an empty Task if @p tasks is empty
             */
            Task make_idle_dispatch_task(std::vector<std::shared_ptr<Reaction>>&& tasks);

            friend class ExternalWaiterRegistration;
            void unregister_external_waiter();

            // The scheduler parent of this pool
            Scheduler& scheduler;

            /// If running is false this means the pool is shutting down and no more tasks will be accepted
            bool running = true;
            /// If accept is false this pool will no longer accept new tasks.
            /// Atomic so that producers on the fast path can check it without taking the pool mutex.
            std::atomic<bool> accept{true};

            /// The threads which are running in this thread pool
            std::vector<std::unique_ptr<std::thread>> threads;

            /// Priority-bucketed task queues. Each bucket holds either an MPMC TaskQueue
            /// (for pools with multiple worker threads) or an MPSCQueue (for pools that are
            /// known to be single-consumer, e.g. MainThread or the Trace pool). The choice
            /// is made at construction based on `descriptor->concurrency`.
            std::array<std::unique_ptr<queue::Queue<Task>>, queue::PRIORITY_BUCKETS> buckets;
            /// Number of tasks submitted but not yet dequeued
            std::atomic<std::size_t> pending_tasks{0};
            /// Number of tasks parked outside the pool (e.g. waiting on a Group token) that point at this pool
            std::atomic<std::size_t> external_waiters{0};
            /// Latched "an external waiter was parked for this pool since you last polled".
            ///
            /// Consumed (cleared to false) at the top of every get_task iteration purely to WAKE a
            /// sleeping worker so it re-checks its queue; it does NOT by itself force an idle fire.
            /// Whether this is actually an idle situation is decided by the normal dequeue-first
            /// path: if the parked waiter has since become runnable and been drained into this
            /// pool's queue (e.g. a Sync group released its token), the worker dequeues and runs it
            /// with no idle epoch. Only if the queue is genuinely empty after dequeuing does the
            /// !got path fire idle, preserving the cross-pool idle-wake / deadlock-break behavior
            /// without prematurely firing idle while real work is still pending.
            ///
            /// This is only ever set when idle_relevant() is true (some idle reaction could fire
            /// on this pool), so on the hot contended path with no idle reactions the latch stays
            /// false and the whole mechanism compiles down to a couple of relaxed atomic loads.
            std::atomic<bool> pending_idle{false};
            /// Number of idle reactions bound directly to this pool (on<Idle<ThisPool>>).
            /// Used by idle_relevant() to cheaply gate the pending_idle machinery.
            std::atomic<std::size_t> idle_task_count{0};

            /**
             * Whether firing an idle epoch on this pool could actually run a reaction.
             *
             * True if there is an idle reaction bound to this pool, or any global idle reaction
             * (which fires when all pools go idle, so any pool may be the last to idle and trigger
             * it). When false, parking an external waiter does not need to wake the pool to fire
             * idle, which keeps the hot Sync-contended submission path free of extra synchronisation.
             */
            bool idle_relevant() const;
            /// A boolean which is set to true when the queue is modified and set to false when there was no work to do
            bool live = true;
            /// True when this pool's buckets use MPSCQueue (single consumer).
            bool single_consumer = false;
            /// Worker thread that owns MPSC dequeue; default until run() sets it.
            std::thread::id consumer_thread_id;
            /// Set by a non-consumer FORCE stop to request the worker discard queued tasks.
            std::atomic<bool> discard_queues_requested{false};

            /// The mutex which protects idle tasks and the live flag
            mutable std::mutex mutex;
            /// The condition variable which threads wait on if they can't get a task
            std::condition_variable condition;

            /// The number of active threads in this pool
            std::atomic<int> active{0};
            /// The idle tasks for this pool
            std::vector<std::shared_ptr<Reaction>> idle_tasks;

            /// The lock which holds the idle state for the specific thread in the pool
            std::map<std::thread::id, std::unique_ptr<Lock>> thread_idle;

            /// When this lock is held, the pool is considered idle
            /// The idle status will be removed when a non idle task is retrieved from the queue
            /// Or when another thread pool notifies this pool, giving its chance at global idle to this pool
            std::unique_ptr<Lock> pool_idle = nullptr;

            /// A thread local pointer to the current pool this thread is running in
            static thread_local Pool* current_pool;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

            friend class Scheduler;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_POOL_HPP
