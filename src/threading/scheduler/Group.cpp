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
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "../../id.hpp"
#include "../../util/GroupDescriptor.hpp"
#include "../ReactionTask.hpp"
#include "Pool.hpp"

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
            int prev_tokens          = 0;
            bool was_locked          = false;

            /*mutex scope*/ {
                const std::lock_guard<std::mutex> lock(group.mutex);
                if (handle->locked) {
                    handle->locked = false;
                    prev_tokens = group.tokens.fetch_add(1, std::memory_order_acq_rel);
                    was_locked  = true;
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

            // If a fast-path waiter was queued (tokens were already negative before our release),
            // drain one waiter to claim the slot we just freed.
            if (was_locked && prev_tokens < 0) {
                group.drain_one_to_pool();
                return;
            }

            // Otherwise: no fast waiter was directly entitled. If slow_pending is now 0 and a
            // token is available, give it to any fast waiter we have so they don't get stranded.
            if (was_locked && group.slow_pending.load(std::memory_order_acquire) == 0) {
                while (true) {
                    int expected = group.tokens.load(std::memory_order_acquire);
                    if (expected <= 0) {
                        break;
                    }
                    if (group.tokens.compare_exchange_weak(expected,
                                                           expected - 1,
                                                           std::memory_order_acq_rel)) {
                        if (!group.drain_one_to_pool()) {
                            group.tokens.fetch_add(1, std::memory_order_release);
                        }
                        break;
                    }
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

        bool Group::try_submit(std::unique_ptr<ReactionTask>&& task,
                               const std::shared_ptr<Pool>& pool,
                               const bool& clear_idle) {
            // Don't jump ahead of multi-group waiters; if any exist, queue ourselves.
            if (slow_pending.load(std::memory_order_acquire) == 0) {
                int expected = tokens.load(std::memory_order_acquire);
                while (expected > 0) {
                    if (tokens.compare_exchange_weak(expected, expected - 1, std::memory_order_acq_rel)) {
                        if (slow_pending.load(std::memory_order_acquire) > 0) {
                            // Restore the token and fall through to enqueueing.
                            release_token();
                            break;
                        }
                        pool->submit({std::move(task), make_running_lock()}, clear_idle);
                        return true;
                    }
                }
            }

            const std::size_t bucket = queue::priority_index(task->priority);
            pool->register_external_waiter();
            wait_buckets[bucket].enqueue(WaitEntry{std::move(task), pool, clear_idle});

            // Reserve a slot in the signed counter; if a token was still available, run a waiter now.
            const int prev = tokens.fetch_sub(1, std::memory_order_acq_rel);
            if (prev > 0) {
                if (slow_pending.load(std::memory_order_acquire) > 0) {
                    // Hand the token back so the slow path can pick it up.
                    release_token();
                }
                else {
                    drain_one_to_pool();
                }
            }

            return false;
        }

        void Group::release_token() {
            const int prev = tokens.fetch_add(1, std::memory_order_acq_rel);

            // If a slow-path waiter exists give them first chance.
            if (slow_pending.load(std::memory_order_acquire) > 0) {
                notify_slow_path();
                return;
            }

            // A fast-path waiter has already decremented; hand them the slot.
            if (prev < 0) {
                drain_one_to_pool();
            }
        }

        void Group::notify_slow_path() {
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

        bool Group::drain_one_to_pool() {
            WaitEntry entry;
            for (std::size_t bucket = 0; bucket < queue::PRIORITY_BUCKETS; ++bucket) {
                if (wait_buckets[bucket].try_dequeue(entry)) {
                    auto pool = entry.pool;
                    pool->submit({std::move(entry.task), make_running_lock()}, entry.clear_idle, /*force=*/true);
                    pool->unregister_external_waiter();
                    return true;
                }
            }
            return false;
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

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
