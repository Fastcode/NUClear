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
#include <vector>

#include "../../id.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        Group::LockHandle::LockHandle(const NUClear::id_t& task_id, const int& priority, std::function<void()> notify)
            : task_id(task_id), priority(priority), notify(std::move(notify)) {}

        Group::GroupLock::GroupLock(Group& group, std::shared_ptr<LockHandle> handle)
            : group(group), handle(std::move(handle)) {}

        Group::GroupLock::~GroupLock() {
            // The notify targets may be trying to lock the group
            // If we try to notify them while holding the lock ourself we will deadlock
            // So extract the notify targets and notify them after we release the lock
            std::vector<std::shared_ptr<LockHandle>> to_notify;

            /*mutex scope*/ {
                const std::lock_guard<std::mutex> lock(group.mutex);
                // Free the token if we held one
                if (handle->locked) {
                    handle->locked = false;
                    group.tokens++;
                }

                // Remove ourself from the queue
                auto it = std::find(group.queue.begin(), group.queue.end(), handle);
                if (it != group.queue.end()) {
                    group.queue.erase(it);
                }

                // Notify any tasks that can lock and hasn't been notified
                int free_tokens = group.tokens;
                for (const auto& h : group.queue) {
                    // Unlocked tasks would consume a token
                    free_tokens -= h->locked ? 0 : 1;

                    // Any tasks that are not locked and have not been notified should be notified
                    if (free_tokens >= 0 && !h->locked && !h->notified) {
                        h->notified = true;
                        to_notify.push_back(h);
                    }
                }
            }

            // Notify all the tasks that can now lock
            for (const auto& h : to_notify) {
                h->notify();
            }
        }

        bool Group::GroupLock::lock() {
            // If already locked then return true
            if (handle->locked) {
                return true;
            }

            const std::lock_guard<std::mutex> lock(group.mutex);

            int free = group.tokens;
            for (const auto& h : group.queue) {
                // Unlocked tasks would consume a token
                free -= h->locked ? 0 : 1;

                // Ran out of free tokens (the 0th token is the last one)
                if (free < 0) {
                    return false;
                }
                if (h == handle) {
                    handle->locked = true;
                    group.tokens--;
                    return true;
                }
            }

            return false;
        }

        Group::Group(std::shared_ptr<const util::GroupDescriptor> descriptor) : descriptor(std::move(descriptor)) {}

        std::unique_ptr<Lock> Group::lock(const NUClear::id_t& task_id,
                                          const int& priority,
                                          const std::function<void()>& notify) {

            auto handle = std::make_shared<LockHandle>(task_id, priority, notify);

            // Insert sorted into the queue
            const std::lock_guard<std::mutex> lock(mutex);
            queue.insert(std::lower_bound(queue.begin(), queue.end(), handle), handle);

            // Unnotify any tasks that are beyond the lock window
            int free = tokens;
            for (const auto& h : queue) {

                // Unlocked tasks would consume a token
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
