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
#include "Group.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "../../id.hpp"
#include "../../util/GroupDescriptor.hpp"
#include "../ReactionTask.hpp"
#include "Lock.hpp"
#include "Pool.hpp"
#include "queue/Priority.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        Group::LockHandle::LockHandle(const NUClear::id_t& task_id, const int& priority, std::function<void()> notify)
            : task_id(task_id), priority(priority), notify(std::move(notify)) {}

        Group::RunningLock::RunningLock(Group& group, std::shared_ptr<Group> group_keepalive)
            : group(group), keepalive(std::move(group_keepalive)) {}

        Group::RunningLock::~RunningLock() {
            group.release_token();
        }

        bool Group::RunningLock::lock() {
            return true;
        }

        Group::GroupLock::GroupLock(Group& group, std::shared_ptr<LockHandle> handle)
            : group(group), handle(std::move(handle)) {}

        Group::GroupLock::~GroupLock() {
            std::vector<std::shared_ptr<LockHandle>> to_notify;
            bool removed_from_queue = false;
            bool was_locked         = false;
            int prev_tokens         = 0;

            /*mutex scope*/ {
                const std::lock_guard<std::mutex> lock(group.mutex);
                if (handle->locked) {
                    handle->locked = false;
                    prev_tokens    = group.tokens.fetch_add(1, std::memory_order_acq_rel);
                    was_locked     = true;
                }

                auto it = std::find(group.queue.begin(), group.queue.end(), handle);
                if (it != group.queue.end()) {
                    group.queue.erase(it);
                    removed_from_queue = true;
                }

                int free_tokens = group.tokens.load(std::memory_order_relaxed);
                for (const auto& h : group.queue) {
                    free_tokens -= h->locked ? 0 : 1;

                    if (free_tokens >= 0 && !h->locked && !h->notified) {
                        h->notified = true;
                        to_notify.push_back(h);
                    }
                }
            }

            if (removed_from_queue) {
                group.slow_pending.fetch_sub(1, std::memory_order_acq_rel);
            }

            for (const auto& h : to_notify) {
                h->notify();
            }

            // A negative pre-release count means a fast-path waiter has ALREADY reserved a slot (its
            // park_reconcile did the fetch_sub) and is parked waiting to be handed back in. It owns
            // the slot we just freed and MUST be drained even when slow_pending > 0, otherwise a
            // multi-group slow waiter that needs this slot to free up deadlocks against it. The
            // drained waiter is normally already counted (slot true), making the drain token-neutral;
            // if we reach a not-yet-counted head waiter it now consumes the freed slot and we owe its
            // single decrement.
            if (was_locked && prev_tokens < 0) {
                const DrainResult drained = group.drain_one_to_pool();
                if (drained.drained && drained.uncounted) {
                    group.tokens.fetch_sub(1, std::memory_order_acq_rel);
                }
                return;
            }

            // Otherwise no committed fast waiter is owed this slot. Hand it to a single parked
            // fast-path waiter, but only once any pending slow-path waiters have been given priority
            // (slow_pending == 0). Draining exactly one waiter per freed token keeps the running
            // count bounded by concurrency: a finishing/releasing task frees one slot and starts at
            // most one parked task, which in turn frees its slot on completion and continues the
            // cascade. If the drained waiter had not yet counted itself (it was mid publish/reconcile
            // when an opportunistic drain reached it, i.e. the race from the lock-free bug) this
            // drain owes its single token decrement; otherwise the drain is token-neutral.
            if (was_locked && group.slow_pending.load(std::memory_order_acquire) == 0) {
                const DrainResult drained = group.drain_one_to_pool();
                if (drained.drained && drained.uncounted) {
                    group.tokens.fetch_sub(1, std::memory_order_acq_rel);
                }
            }
        }

        bool Group::GroupLock::lock() {
            if (handle->locked) {
                return true;
            }

            const std::lock_guard<std::mutex> lock(group.mutex);

            int free = group.tokens.load(std::memory_order_relaxed);
            for (const auto& h : group.queue) {
                free -= h->locked ? 0 : 1;

                if (free < 0) {
                    return false;
                }
                if (h == handle) {
                    handle->locked = true;
                    group.tokens.fetch_sub(1, std::memory_order_release);
                    return true;
                }
            }

            return false;
        }

        Group::Group(std::shared_ptr<const util::GroupDescriptor> descriptor)
            : descriptor(std::move(descriptor)), tokens(this->descriptor->concurrency) {}

        Group::~Group() {
            // Drain any waiters still parked in the fast-path buckets. WaitEntry holds an
            // ExternalWaiterRegistration that unregisters from the pool on destruction.
            WaitEntry entry;
            for (auto& bucket : wait_buckets) {
                while (bucket.try_dequeue(entry)) {
                    entry = WaitEntry{};
                }
            }
        }

        std::unique_ptr<Lock> Group::try_acquire_running_lock() {
            if (slow_pending.load(std::memory_order_acquire) > 0) {
                return nullptr;
            }
            int expected = tokens.load(std::memory_order_acquire);
            while (expected > 0) {
                if (tokens.compare_exchange_weak(expected, expected - 1, std::memory_order_acq_rel)) {
                    if (slow_pending.load(std::memory_order_acquire) > 0) {
                        // A multi-group waiter slipped in; restore the token and back off.
                        release_token();
                        return nullptr;
                    }
                    return make_running_lock();
                }
            }
            return nullptr;
        }

        bool Group::try_submit(std::unique_ptr<ReactionTask>&& task, Pool* pool, const bool& clear_idle) {
            // Don't jump ahead of multi-group waiters; if any exist, queue ourselves.
            if (slow_pending.load(std::memory_order_acquire) == 0) {
                int expected = tokens.load(std::memory_order_acquire);
                bool done    = false;
                while (!done && expected > 0) {
                    if (tokens.compare_exchange_weak(expected, expected - 1, std::memory_order_acq_rel)) {
                        if (slow_pending.load(std::memory_order_acquire) > 0) {
                            // Restore the token and fall through to enqueueing.
                            release_token();
                            done = true;
                        }
                        else {
                            pool->submit({std::move(task), make_running_lock()}, clear_idle);
                            return true;
                        }
                    }
                }
            }

            const std::shared_ptr<std::atomic<bool>> slot = park_publish(std::move(task), pool, clear_idle);
            park_reconcile(slot);
            return false;
        }

        std::shared_ptr<std::atomic<bool>> Group::park_publish(std::unique_ptr<ReactionTask>&& task,
                                                               Pool* pool,
                                                               const bool& clear_idle) noexcept {
            auto slot                = std::make_shared<std::atomic<bool>>(false);
            const std::size_t bucket = queue::priority_index(task->priority);
            ExternalWaiterRegistration external_waiter;
            if (pool != nullptr) {
                external_waiter = pool->register_external_waiter();
            }
            wait_buckets[bucket].enqueue(
                WaitEntry{std::move(task), pool, clear_idle, slot, std::move(external_waiter)});
            return slot;
        }

        void Group::park_reconcile(const std::shared_ptr<std::atomic<bool>>& slot) noexcept {
            // Reserve a slot in the signed counter. This is done unconditionally so that a later
            // release always sees prev < 0 and hands us back in: it is the no-lost-wakeup mechanism.
            const int prev = tokens.fetch_sub(1, std::memory_order_acq_rel);

            // A token was free, but a multi-group slow waiter is pending: the slow path has priority.
            // Hand the token straight back and stay parked UNCOUNTED -- we deliberately do NOT claim
            // the arbiter slot. A later drain then owes our single decrement (paired with our eventual
            // RunningLock release) and runs us once the slow path has cleared. This is what stops a
            // single-group fast submit from jumping ahead of an older multi-group waiter (see
            // dsl/SyncMulti); leaving the slot unclaimed keeps the drain's accounting exact.
            if (prev > 0 && slow_pending.load(std::memory_order_acquire) > 0) {
                release_token();
                return;
            }

            // Claim responsibility for this waiter's single token decrement. Whoever flips the arbiter
            // from false to true owns the decrement; if an opportunistic drainer already flipped it, it
            // has both started us running and accounted the token it kept, so our fetch_sub above is a
            // phantom -- undo it and leave.
            if (slot->exchange(true, std::memory_order_acq_rel)) {
                release_token();
                return;
            }

            if (prev > 0) {
                // A token was free (and no slow waiter took priority), so hand it to a parked waiter
                // (possibly us). If that waiter had not yet counted itself this drain owes its single
                // decrement.
                const DrainResult drained = drain_one_to_pool();
                if (drained.drained && drained.uncounted) {
                    tokens.fetch_sub(1, std::memory_order_acq_rel);
                }
            }

            // The destination pool's "pending idle" latch was set by register_external_waiter
            // above; that path also notifies one waiting worker so a pool that is parked on its
            // condition variable can act on the latch immediately. See Pool::register_external_waiter
            // and Pool::get_task for the full mechanism (it preserves the OLD scheduler's invariant
            // that a parked waiter always triggered exactly one idle fire on its destination pool,
            // even when the worker is preempted past the natural idle window).
        }

        void Group::release_token() noexcept {
            const int prev = tokens.fetch_add(1, std::memory_order_acq_rel);

            // A negative pre-release count means at least one fast-path waiter has ALREADY reserved a
            // slot (its park_reconcile did the fetch_sub) and is parked waiting to be handed back in.
            // That waiter committed before any slow waiter and now owns the slot we just freed, so it
            // MUST be drained even when slow_pending > 0: stranding it would deadlock a multi-group
            // slow waiter that needs this very slot to become free again. The drained waiter is
            // normally already counted (its slot is true), so the drain is token-neutral; if we
            // instead reach a not-yet-counted head waiter it now consumes the freed slot, so we owe
            // its single decrement.
            if (prev < 0) {
                const DrainResult drained = drain_one_to_pool();
                if (drained.drained && drained.uncounted) {
                    tokens.fetch_sub(1, std::memory_order_acq_rel);
                }
                return;
            }

            // No committed fast waiter is owed this slot. Give any slow-path waiter first chance.
            if (slow_pending.load(std::memory_order_acquire) > 0) {
                notify_slow_path();
                return;
            }

            // Otherwise hand the one freed slot to a single parked fast-path waiter. Draining exactly
            // one per freed token bounds the running count by concurrency and lets each completing
            // task continue the cascade. If the drained waiter had not yet counted itself this drain
            // owes its single token decrement (it consumes the slot we just freed); for an
            // already-counted waiter the drain is token-neutral.
            const DrainResult drained = drain_one_to_pool();
            if (drained.drained && drained.uncounted) {
                tokens.fetch_sub(1, std::memory_order_acq_rel);
            }
        }

        void Group::notify_slow_path() noexcept {
            std::vector<std::shared_ptr<LockHandle>> to_notify;
            /*mutex scope*/ {
                const std::lock_guard<std::mutex> lock(mutex);
                int free_tokens = tokens.load(std::memory_order_relaxed);
                for (const auto& h : queue) {
                    free_tokens -= h->locked ? 0 : 1;
                    if (free_tokens >= 0 && !h->locked && !h->notified) {
                        h->notified = true;
                        to_notify.push_back(h);
                    }
                }
            }
            for (const auto& h : to_notify) {
                h->notify();
            }
        }

        Group::DrainResult Group::drain_one_to_pool() noexcept {
            WaitEntry entry;
            for (std::size_t bucket = 0; bucket < queue::PRIORITY_BUCKETS; ++bucket) {
                if (wait_buckets[bucket].try_dequeue(entry)) {
                    Pool* pool = entry.pool;
                    // Claim the waiter's single token decrement. If the slot was still false the
                    // waiter has not counted itself yet (it is mid publish/reconcile, or it handed
                    // its token back to the slow path), so this drain is responsible for the -1 and
                    // the waiter's park_reconcile() will observe the slot and skip. If it was already
                    // true the waiter is counted and this drain is token-neutral.
                    const bool uncounted = !entry.slot->exchange(true, std::memory_order_acq_rel);
                    auto running_lock    = make_running_lock();
#ifdef NUCLEAR_GROUP_TEST_API
                    if (test_capture_drains_) {
                        test_captured_drains_.push_back({std::move(entry.task), std::move(running_lock)});
                        return {true, uncounted};
                    }
#endif
                    pool->submit({std::move(entry.task), std::move(running_lock)}, entry.clear_idle, /*force=*/true);
                    return {true, uncounted};
                }
            }
            return {};
        }

        std::unique_ptr<Lock> Group::make_running_lock() {
            return std::make_unique<RunningLock>(*this, shared_from_this());
        }

        std::unique_ptr<Lock> Group::lock(const NUClear::id_t& task_id,
                                          const int& priority,
                                          const std::function<void()>& notify) {

            auto handle = std::make_shared<LockHandle>(task_id, priority, notify);

            slow_pending.fetch_add(1, std::memory_order_acq_rel);

            const std::lock_guard<std::mutex> lock(mutex);
            queue.insert(std::lower_bound(queue.begin(), queue.end(), handle), handle);

            int free = tokens.load(std::memory_order_relaxed);
            for (const auto& h : queue) {
                free -= h->locked ? 0 : 1;
                if (free < 0) {
                    h->notified = false;
                }
            }

            return std::make_unique<GroupLock>(*this, handle);
        }

#ifdef NUCLEAR_GROUP_TEST_API
        int Group::TestAccess::tokens(const Group& group) {
            return group.tokens.load(std::memory_order_acquire);
        }

        std::shared_ptr<std::atomic<bool>> Group::TestAccess::park_publish(Group& group,
                                                                           std::unique_ptr<ReactionTask>&& task,
                                                                           Pool* pool,
                                                                           const bool clear_idle) {
            return group.park_publish(std::move(task), pool, clear_idle);
        }

        void Group::TestAccess::park_reconcile(Group& group, const std::shared_ptr<std::atomic<bool>>& slot) {
            group.park_reconcile(slot);
        }

        std::unique_ptr<Lock> Group::TestAccess::try_acquire_running_lock(Group& group) {
            return group.try_acquire_running_lock();
        }

        void Group::TestAccess::set_capture_drains(Group& group, const bool capture) {
            group.test_capture_drains_ = capture;
        }

        std::vector<Group::CapturedDrain> Group::TestAccess::take_captured_drains(Group& group) {
            std::vector<CapturedDrain> captured;
            captured.swap(group.test_captured_drains_);
            return captured;
        }
#endif

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
