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
#ifndef NUCLEAR_THREADING_SCHEDULER_QUEUE_SEMAPHORE_HPP
#define NUCLEAR_THREADING_SCHEDULER_QUEUE_SEMAPHORE_HPP

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            /**
             * Counting semaphore with an atomic fast path and mutex/condition_variable slow path.
             *
             * A negative count indicates the number of threads blocked in wait().
             */
            class Semaphore {
            public:
                Semaphore()  = default;
                ~Semaphore() = default;

                Semaphore(const Semaphore&)            = delete;
                Semaphore& operator=(const Semaphore&) = delete;
                Semaphore(Semaphore&&)                 = delete;
                Semaphore& operator=(Semaphore&&)      = delete;

                void signal(int n = 1) {
                    const int previous = count.fetch_add(n, std::memory_order_release);
                    if (previous < 0) {
                        const std::lock_guard<std::mutex> lock(mutex);
                        const int waiters = std::min(n, -previous);
                        for (int i = 0; i < waiters; ++i) {
                            cv.notify_one();
                        }
                    }
                }

                void wait() {
                    if (count.fetch_sub(1, std::memory_order_acq_rel) > 0) {
                        return;
                    }

                    std::unique_lock<std::mutex> lock(mutex);
                    while (count.load(std::memory_order_acquire) < 0) {
                        cv.wait(lock);
                    }
                }

                bool try_wait() {
                    int expected = count.load(std::memory_order_acquire);
                    while (expected > 0) {
                        if (count.compare_exchange_weak(expected, expected - 1, std::memory_order_acq_rel)) {
                            return true;
                        }
                    }
                    return false;
                }

            private:
                std::atomic<int> count{0};
                std::mutex mutex;
                std::condition_variable cv;
            };

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_QUEUE_SEMAPHORE_HPP
