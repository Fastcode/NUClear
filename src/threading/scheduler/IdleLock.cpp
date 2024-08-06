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
#include "IdleLock.hpp"

#include <atomic>
#include <utility>

namespace NUClear {
namespace threading {
    namespace scheduler {

        template <typename Func>
        static std::pair<IdleLock::semaphore_t, IdleLock::semaphore_t>
            apply_cas(std::atomic<IdleLock::semaphore_t>& active, Func func, const std::memory_order& memory_order) {
            // Perform a CAS loop to try and update the active count and lock status
            while (true) {
                IdleLock::semaphore_t current = active.load(std::memory_order_relaxed);
                IdleLock::semaphore_t update  = func(current);
                if (active.compare_exchange_weak(current, update, memory_order, std::memory_order_relaxed)) {
                    return std::make_pair(current, update);
                }
            }

            return {};  // unreachable
        }

        IdleLock::IdleLock(std::atomic<IdleLock::semaphore_t>& active) : active(active) {

            auto values = apply_cas(
                active,
                [](const semaphore_t& v) { return v == 1 ? MASK : v - 1; },
                std::memory_order_acquire);

            // If the lock was not already locked and the new value is locked
            locked = ((values.first & MASK) != MASK) && ((values.second & MASK) == MASK);
        }

        IdleLock::~IdleLock() {
            if (locked) {
                apply_cas(active, [](const semaphore_t& c) { return (c & ~MASK) + 1; }, std::memory_order_release);
            }
            else {
                active.fetch_add(1, std::memory_order_release);
            }
        }

        bool IdleLock::lock() {
            return locked;
        }

        IdleLockPair::IdleLockPair(std::atomic<IdleLock::semaphore_t>& local_active,
                                   std::atomic<IdleLock::semaphore_t>& global_active)
            : local(local_active), global(global_active) {}

        bool IdleLockPair::lock() {
            return local.lock() || global.lock();
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
