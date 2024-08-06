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
#include "GroupLock.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "Pool.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        GroupLock::GroupLock(const std::shared_ptr<Group>& group, const std::function<void()>& notifier)
            : group(group), notifier(notifier) {}

        GroupLock::~GroupLock() {
            if (locked) {
                group->tokens.fetch_add(1, std::memory_order_release);
                group->notify();
            }
        }

        bool GroupLock::lock() {
            // Once a lock has been obtained it remains until the lock is destroyed
            if (locked) {
                return true;
            }

            // Use atomic CAS to ensure that the available token count is decremented atomically
            for (int expected = group->tokens.load(std::memory_order_relaxed); expected > 0;
                 expected     = group->tokens.load(std::memory_order_relaxed)) {
                if (group->tokens.compare_exchange_weak(expected,
                                                        expected - 1,
                                                        std::memory_order_acquire,
                                                        std::memory_order_relaxed)) {
                    locked = true;
                    return true;
                }
            }

            // If we failed to lock, then we need to add ourselves to the group's watchers unless we already had
            if (!watcher_handle || watcher_handle->called) {
                watcher_handle = group->add_watcher(notifier);
            }

            return false;
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
