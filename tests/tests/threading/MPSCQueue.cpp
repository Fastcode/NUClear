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
#include "threading/scheduler/queue/MPSCQueue.hpp"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            namespace {
                /// Counts how many instances are currently alive so a test can detect skipped
                /// destructors. Construction (incl. copy/move) increments; destruction decrements.
                std::atomic<int> live_tracker_count{0};  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

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

            SCENARIO("An MPSCQueue destroyed while non-empty runs the destructors of its remaining items",
                     "[threading][queue][MPSCQueue]") {
                GIVEN("An MPSCQueue filled across several blocks then only partially drained") {
                    live_tracker_count.store(0, std::memory_order_relaxed);

                    WHEN("The queue is destroyed with items still enqueued") {
                        {
                            MPSCQueue<LiveTracker> queue;
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

            SCENARIO("An MPSCQueue accepts copy-enqueued const payloads", "[threading][queue][MPSCQueue]") {
                GIVEN("An empty MPSCQueue<int>") {
                    MPSCQueue<int> queue;

                    WHEN("A value is enqueued via the const lvalue overload") {
                        const int value = 7;
                        queue.enqueue(value);

                        THEN("The same value is dequeued") {
                            int out = 0;
                            CHECK(queue.try_dequeue(out));
                            CHECK(out == 7);
                            CHECK_FALSE(queue.try_dequeue(out));
                        }
                    }
                }
            }

            SCENARIO("An MPSCQueue consumer waits while a producer links the next block",
                     "[threading][queue][MPSCQueue]") {
                GIVEN("An MPSCQueue with one full block and a producer about to overflow it") {
                    MPSCQueue<int> queue;
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

                        THEN("Every integer is delivered in order despite the block rollover race") {
                            CHECK(producer_done.load(std::memory_order_acquire));
                            CHECK(in_order);
                            int discard = 0;
                            CHECK_FALSE(queue.try_dequeue(discard));
                        }
                    }
                }
            }

            SCENARIO("An MPSCQueue used by a single producer and single consumer preserves FIFO order",
                     "[threading][queue][MPSCQueue]") {
                GIVEN("An empty MPSCQueue<int>") {
                    MPSCQueue<int> queue;

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
                        }
                    }
                }
            }

            SCENARIO("An MPSCQueue can store move-only payloads", "[threading][queue][MPSCQueue]") {
                GIVEN("An MPSCQueue of std::unique_ptr<int>") {
                    MPSCQueue<std::unique_ptr<int>> queue;

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

            SCENARIO("An MPSCQueue handles many enqueues from one thread followed by many dequeues",
                     "[threading][queue][MPSCQueue]") {
                GIVEN("An MPSCQueue with 5000 sequentially enqueued integers") {
                    MPSCQueue<int> queue;
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
                            int discard = 0;
                            CHECK_FALSE(queue.try_dequeue(discard));
                        }
                    }
                }
            }

            // Stress test for the MPSC contract: many producers race to enqueue while a single consumer
            // drains. We tag each item with (producer_id, sequence_no) so we can assert per-producer FIFO
            // is preserved even though cross-producer ordering is intentionally undefined.
            SCENARIO("An MPSCQueue used by many producers and one consumer preserves per-producer FIFO",
                     "[threading][queue][MPSCQueue]") {
                GIVEN("Eight producer threads each enqueueing 2000 (producer_id, sequence) pairs") {
                    constexpr int items_per_producer = 2000;
                    constexpr int producers          = 8;

                    MPSCQueue<std::pair<int, int>> queue;
                    std::atomic<int>               produced{0};

                    WHEN("A single consumer drains every item that the producers emit") {
                        std::vector<std::thread> producer_threads;
                        producer_threads.reserve(producers);
                        for (int p = 0; p < producers; ++p) {
                            producer_threads.emplace_back([&, p]() {
                                for (int i = 0; i < items_per_producer; ++i) {
                                    queue.enqueue({p, i});
                                    produced.fetch_add(1, std::memory_order_relaxed);
                                }
                            });
                        }

                        std::vector<int> per_producer_last(producers, -1);
                        bool             per_producer_fifo_ok = true;
                        int              consumed             = 0;
                        while (consumed < producers * items_per_producer) {
                            std::pair<int, int> value{};
                            if (queue.try_dequeue(value)) {
                                if (value.second != per_producer_last[value.first] + 1) {
                                    per_producer_fifo_ok = false;
                                }
                                per_producer_last[value.first] = value.second;
                                ++consumed;
                            }
                            else {
                                std::this_thread::yield();
                            }
                        }

                        for (auto& thread : producer_threads) {
                            thread.join();
                        }

                        THEN("Every item appears exactly once and per-producer order is preserved") {
                            CHECK(produced.load() == producers * items_per_producer);
                            CHECK(consumed == producers * items_per_producer);
                            CHECK(per_producer_fifo_ok);
                        }
                    }
                }
            }

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
