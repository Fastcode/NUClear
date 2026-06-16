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
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            namespace {
                /// Counts how many instances are currently alive so a test can detect skipped
                /// destructors. Construction (incl. copy/move) increments; destruction decrements.
                std::atomic<int> live_tracker_count{0};

                struct LiveTracker {
                    int value;
                    explicit LiveTracker(int v = 0) : value(v) {
                        live_tracker_count.fetch_add(1, std::memory_order_relaxed);
                    }
                    LiveTracker(const LiveTracker& other) : value(other.value) {
                        live_tracker_count.fetch_add(1, std::memory_order_relaxed);
                    }
                    LiveTracker(LiveTracker&& other) noexcept : value(other.value) {
                        live_tracker_count.fetch_add(1, std::memory_order_relaxed);
                    }
                    LiveTracker& operator=(const LiveTracker&) = default;
                    LiveTracker& operator=(LiveTracker&&) noexcept = default;
                    ~LiveTracker() {
                        live_tracker_count.fetch_sub(1, std::memory_order_relaxed);
                    }
                };
            }  // namespace

            SCENARIO("A TaskQueue destroyed while non-empty runs the destructors of its remaining items",
                     "[threading][queue][TaskQueue]") {
                GIVEN("A TaskQueue filled across several blocks then only partially drained") {
                    live_tracker_count.store(0, std::memory_order_relaxed);

                    WHEN("The queue is destroyed with items still enqueued") {
                        {
                            TaskQueue<LiveTracker> queue;
                            for (int i = 0; i < 200; ++i) {
                                queue.enqueue(LiveTracker(i));
                            }
                            /*drain a few*/ {
                                LiveTracker sink(-1);
                                for (int i = 0; i < 10; ++i) {
                                    REQUIRE(queue.try_dequeue(sink));
                                }
                            }

                            // 190 elements remain live inside the queue's blocks.
                            CHECK(live_tracker_count.load(std::memory_order_relaxed) == 190);
                        }

                        THEN("Every still-enqueued element has its destructor run") {
                            CHECK(live_tracker_count.load(std::memory_order_relaxed) == 0);
                        }
                    }
                }
            }

            SCENARIO("A TaskQueue accepts copy-enqueued const payloads", "[threading][queue][TaskQueue]") {
                GIVEN("An empty TaskQueue<int>") {
                    TaskQueue<int> queue;

                    WHEN("A value is enqueued via the const lvalue overload") {
                        const int value = 7;
                        queue.enqueue(value);

                        THEN("The same value is dequeued and the queue reports empty") {
                            int out = 0;
                            CHECK(queue.try_dequeue(out));
                            CHECK(out == 7);
                            CHECK(queue.empty());
                        }
                    }
                }
            }

            SCENARIO("A TaskQueue empty() is false while a later block still holds items",
                     "[threading][queue][TaskQueue]") {
                GIVEN("A TaskQueue whose first block is fully drained but a second block is populated") {
                    TaskQueue<int> queue;
                    for (int i = 0; i < 65; ++i) {
                        queue.enqueue(i);
                    }
                    for (int i = 0; i < 64; ++i) {
                        int discard = -1;
                        REQUIRE(queue.try_dequeue(discard));
                        CHECK(discard == i);
                    }

                    WHEN("empty() is queried before the remaining item is dequeued") {
                        THEN("The queue is not empty") {
                            CHECK_FALSE(queue.empty());
                            int last = -1;
                            CHECK(queue.try_dequeue(last));
                            CHECK(last == 64);
                            CHECK(queue.empty());
                        }
                    }
                }
            }

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
                GIVEN("Four producer threads each enqueueing 2000 items and four consumer threads draining") {
                    constexpr int items_per_producer = 2000;
                    constexpr int producers          = 4;
                    constexpr int consumers          = 4;

                    TaskQueue<int>   queue;
                    std::atomic<int> produced{0};
                    std::atomic<int> consumed{0};

                    WHEN("All producers and consumers run to completion") {
                        std::vector<std::thread> threads;
                        threads.reserve(static_cast<std::size_t>(producers) + static_cast<std::size_t>(consumers));
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

            SCENARIO("A TaskQueue consumer can spin until a producer publishes the first slot of a new block",
                     "[threading][queue][TaskQueue]") {
                GIVEN("A TaskQueue whose head block is fully drained while a producer is linking the next") {
                    TaskQueue<int> queue;
                    for (int i = 0; i < 64; ++i) {
                        queue.enqueue(i);
                    }

                    WHEN("A producer and consumer race across the block boundary") {
                        std::atomic<bool> producer_done{false};
                        std::thread producer([&] {
                            for (int i = 64; i < 128; ++i) {
                                queue.enqueue(i);
                            }
                            producer_done.store(true, std::memory_order_release);
                        });

                        bool in_order = true;
                        for (int expected = 0; expected < 128; ++expected) {
                            int value = -1;
                            while (!queue.try_dequeue(value)) {
                                std::this_thread::yield();
                            }
                            if (value != expected) {
                                in_order = false;
                                break;
                            }
                        }

                        producer.join();

                        THEN("Every integer is delivered and the queue ends empty") {
                            CHECK(producer_done.load(std::memory_order_acquire));
                            CHECK(in_order);
                            CHECK(queue.empty());
                        }
                    }
                }
            }

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
