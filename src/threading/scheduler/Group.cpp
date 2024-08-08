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

namespace NUClear {
namespace threading {
    namespace scheduler {

        Group::LockHandle::LockHandle(const NUClear::id_t& task_id,
                                      const int& priority,
                                      const bool& locked,
                                      const std::function<void()>& notify)
            : task_id(task_id), priority(priority), locked(locked), notify(notify) {}

        Group::GroupLock::GroupLock(Group& group, const std::shared_ptr<LockHandle>& handle)
            : group(group), handle(handle) {}

        Group::GroupLock::~GroupLock() {
            // The notify targets may be trying to lock the group
            // If we try to notify them here we will deadlock
            std::vector<std::shared_ptr<LockHandle>> to_notify;

            /*mutex scope*/ {
                std::lock_guard<std::mutex> lock(group.mutex);
                // Free the token if we held one
                if (handle->locked) {
                    handle->locked = false;
                    group.tokens++;
                }

                // Find our position in the queue and notify any tasks that can now lock which couldn't before
                int free_tokens = group.tokens;
                bool found      = false;
                for (auto it = group.queue.begin(); it != group.queue.end();) {

                    // If this is the lock we are removing, then we have found it and we should remove it
                    if (!found && *it == handle) {
                        found = true;
                        it    = group.queue.erase(it);
                    }
                    else {
                        // Unlocked tasks would consume a token
                        free_tokens -= (*it)->locked ? 0 : 1;

                        if (found) {
                            // Ran out of tokens to give out
                            if (free_tokens < 0) {
                                break;
                            }
                            if (!(*it)->locked) {
                                to_notify.push_back(*it);
                            }
                        }
                        ++it;
                    }
                }
            }

            // Notify all the tasks that can now lock
            for (auto& h : to_notify) {
                h->notify();
            }
        }

        bool Group::GroupLock::lock() {
            // If already locked then return true
            if (handle->locked) {
                return true;
            }

            std::lock_guard<std::mutex> lock(group.mutex);

            int free = group.tokens;
            for (auto& h : group.queue) {
                // Unlocked tasks would consume a token
                free -= h->locked ? 0 : 1;

                // Ran out of free tokens
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

        Group::Group(const util::GroupDescriptor& descriptor) : descriptor(descriptor) {}

        std::unique_ptr<Lock> Group::lock(const NUClear::id_t& task_id,
                                          const int& priority,
                                          const std::function<void()>& notify) {

            auto handle = std::make_shared<LockHandle>(task_id, priority, false, notify);

            // Insert sorted into the queue
            std::lock_guard<std::mutex> lock(mutex);
            queue.insert(std::lower_bound(queue.begin(), queue.end(), handle), handle);

            return std::make_unique<GroupLock>(*this, handle);
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
