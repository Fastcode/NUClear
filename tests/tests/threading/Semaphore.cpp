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
#include "threading/scheduler/queue/Semaphore.hpp"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <thread>

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            SCENARIO("A signal on a semaphore unblocks a thread that is waiting on it",
                     "[threading][queue][Semaphore]") {
                GIVEN("A fresh semaphore with a thread blocked on wait()") {
                    Semaphore         sem;
                    std::atomic<bool> done{false};
                    std::thread       waiter([&]() {
                        sem.wait();
                        done.store(true, std::memory_order_release);
                    });
                    // Give the waiter a moment to actually park on the semaphore before we observe it.
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));

                    WHEN("No signal has been sent yet") {
                        THEN("The waiting thread is still blocked") {
                            CHECK_FALSE(done.load(std::memory_order_acquire));
                        }
                    }

                    WHEN("A signal is sent") {
                        sem.signal();
                        waiter.join();

                        THEN("The waiting thread runs to completion") {
                            CHECK(done.load(std::memory_order_acquire));
                        }
                    }

                    // Whichever WHEN branch ran, make sure the waiter thread is released before this
                    // scope ends so we never leak a joinable std::thread into destruction.
                    if (waiter.joinable()) {
                        sem.signal();
                        waiter.join();
                    }
                }
            }

            SCENARIO("try_wait only succeeds when the semaphore has been signalled",
                     "[threading][queue][Semaphore]") {
                GIVEN("A fresh semaphore") {
                    Semaphore sem;

                    WHEN("try_wait is called before any signal") {
                        THEN("It returns false") {
                            CHECK_FALSE(sem.try_wait());
                        }
                    }

                    WHEN("A signal is sent and try_wait is called twice") {
                        sem.signal();
                        const bool first  = sem.try_wait();
                        const bool second = sem.try_wait();

                        THEN("The first try_wait consumes the signal and the second returns false") {
                            CHECK(first);
                            CHECK_FALSE(second);
                        }
                    }
                }
            }

            SCENARIO("Signals and waits across two threads are conserved one-for-one",
                     "[threading][queue][Semaphore]") {
                GIVEN("A semaphore with a consumer thread issuing many waits") {
                    constexpr int    iterations = 1000;
                    Semaphore        sem;
                    std::atomic<int> completed{0};

                    std::thread consumer([&]() {
                        for (int i = 0; i < iterations; ++i) {
                            sem.wait();
                            completed.fetch_add(1, std::memory_order_relaxed);
                        }
                    });

                    WHEN("The same number of signals are emitted from the producer") {
                        for (int i = 0; i < iterations; ++i) {
                            sem.signal();
                        }
                        consumer.join();

                        THEN("Every signal is matched by exactly one wait completion") {
                            CHECK(completed.load() == iterations);
                        }
                    }
                }
            }

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
