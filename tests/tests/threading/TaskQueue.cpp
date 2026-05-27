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
#include "threading/scheduler/queue/TaskQueue.hpp"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <thread>
#include <vector>

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            SCENARIO("A TaskQueue used by a single producer and a single consumer preserves FIFO order",
                     "[threading][queue][TaskQueue]") {
                GIVEN("An empty TaskQueue<int>") {
                    TaskQueue<int> queue;

                    WHEN("Two values are enqueued in order") {
                        queue.enqueue(1);
                        queue.enqueue(2);

                        THEN("They are dequeued in the same order and the queue is then empty") {
                            int value = 0;
                            CHECK(queue.try_dequeue(value));
                            CHECK(value == 1);
                            CHECK(queue.try_dequeue(value));
                            CHECK(value == 2);
                            CHECK_FALSE(queue.try_dequeue(value));
                            CHECK(queue.empty());
                        }
                    }
                }
            }

            SCENARIO("A TaskQueue can store move-only payloads", "[threading][queue][TaskQueue]") {
                GIVEN("A TaskQueue of std::unique_ptr<int>") {
                    TaskQueue<std::unique_ptr<int>> queue;

                    WHEN("A unique_ptr holding 42 is enqueued") {
                        queue.enqueue(std::make_unique<int>(42));

                        THEN("The same value can be dequeued without copying") {
                            std::unique_ptr<int> value;
                            CHECK(queue.try_dequeue(value));
                            REQUIRE(value != nullptr);
                            CHECK(*value == 42);
                        }
                    }
                }
            }

            SCENARIO("A TaskQueue handles many enqueues from one thread followed by many dequeues",
                     "[threading][queue][TaskQueue]") {
                GIVEN("A TaskQueue with 5000 sequentially enqueued integers") {
                    TaskQueue<int> queue;
                    for (int i = 0; i < 5000; ++i) {
                        queue.enqueue(i);
                    }

                    WHEN("They are all dequeued in turn") {
                        bool sequence_holds = true;
                        for (int i = 0; i < 5000; ++i) {
                            int value = -1;
                            if (!queue.try_dequeue(value) || value != i) {
                                sequence_holds = false;
                                break;
                            }
                        }

                        THEN("Each dequeue returns the next integer in order and the queue is empty") {
                            CHECK(sequence_holds);
                            CHECK(queue.empty());
                        }
                    }
                }
            }

            // Stress test: with multiple producers writing concurrently we cannot assert
            // total ordering across producers, but every item must come out exactly once.
            SCENARIO("A TaskQueue used by many producers and many consumers conserves every item",
                     "[threading][queue][TaskQueue]") {
                GIVEN("Four producer threads each enqueueing 500 items and four consumer threads draining") {
                    constexpr int items_per_producer = 500;
                    constexpr int producers          = 4;
                    constexpr int consumers          = 4;

                    TaskQueue<int>   queue;
                    std::atomic<int> produced{0};
                    std::atomic<int> consumed{0};

                    WHEN("All producers and consumers run to completion") {
                        std::vector<std::thread> threads;
                        for (int p = 0; p < producers; ++p) {
                            threads.emplace_back([&, p]() {
                                for (int i = 0; i < items_per_producer; ++i) {
                                    queue.enqueue(p * items_per_producer + i);
                                    produced.fetch_add(1, std::memory_order_relaxed);
                                }
                            });
                        }
                        for (int c = 0; c < consumers; ++c) {
                            threads.emplace_back([&]() {
                                int value = 0;
                                while (consumed.load(std::memory_order_acquire) < producers * items_per_producer) {
                                    if (queue.try_dequeue(value)) {
                                        consumed.fetch_add(1, std::memory_order_relaxed);
                                    }
                                    else {
                                        std::this_thread::yield();
                                    }
                                }
                            });
                        }

                        for (auto& thread : threads) {
                            thread.join();
                        }

                        THEN("Total produced equals total consumed and the queue ends empty") {
                            CHECK(produced.load() == producers * items_per_producer);
                            CHECK(consumed.load() == producers * items_per_producer);
                            CHECK(queue.empty());
                        }
                    }
                }
            }

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
