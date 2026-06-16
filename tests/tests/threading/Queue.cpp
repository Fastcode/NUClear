/*
 * MIT License
 *
 * Copyright (c) 2026 NUClear Contributors
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
#include "threading/scheduler/queue/TaskQueue.hpp"

#include <atomic>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>

#include "test_util/queue_live_tracker.hpp"

namespace {

template <typename Queue, typename = void>
struct has_empty : std::false_type {};

template <typename Queue>
struct has_empty<Queue, decltype(void(std::declval<const Queue&>().empty()))> : std::true_type {};

template <typename Queue>
void assert_queue_reports_empty(Queue& queue, std::true_type /*has_empty*/) {
    CHECK(queue.empty());
}

template <typename Queue>
void assert_queue_reports_empty(Queue& queue, std::false_type /*has_empty*/) {
    int discard = 0;
    CHECK_FALSE(queue.try_dequeue(discard));
}

template <typename Queue>
void assert_queue_reports_empty(Queue& queue) {
    assert_queue_reports_empty(queue, has_empty<Queue>{});
}

}  // namespace

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            TEMPLATE_TEST_CASE("Scenario: A queue destroyed while non-empty runs the destructors of its remaining items",
                               "[threading][queue]",
                               MPSCQueue<test_util::QueueLiveTracker>,
                               TaskQueue<test_util::QueueLiveTracker>) {
                GIVEN("A queue filled across several blocks then only partially drained") {
                    test_util::queue_live_tracker_count().store(0, std::memory_order_relaxed);

                    WHEN("The queue is destroyed with items still enqueued") {
                        {
                            TestType queue;
                            constexpr std::size_t partial_blocks = 3;
                            constexpr std::size_t partial_extra  = 8;
                            constexpr std::size_t items_enqueued =
                                partial_blocks * TestType::BLOCK_SIZE + partial_extra;
                            constexpr int drain_count = 10;
                            for (std::size_t i = 0; i < items_enqueued; ++i) {
                                queue.enqueue(test_util::QueueLiveTracker(static_cast<int>(i)));
                            }
                            /*drain a few*/ {
                                test_util::QueueLiveTracker sink(-1);
                                for (int i = 0; i < drain_count; ++i) {
                                    REQUIRE(queue.try_dequeue(sink));
                                }
                            }

                            const int remaining =
                                static_cast<int>(items_enqueued) - drain_count;
                            CHECK(test_util::queue_live_tracker_count().load(std::memory_order_relaxed) == remaining);
                        }

                        THEN("Every still-enqueued element has its destructor run") {
                            CHECK(test_util::queue_live_tracker_count().load(std::memory_order_relaxed) == 0);
                        }
                    }
                }
            }

            TEMPLATE_TEST_CASE("Scenario: A queue accepts copy-enqueued const payloads",
                               "[threading][queue]",
                               MPSCQueue<int>,
                               TaskQueue<int>) {
                GIVEN("An empty queue") {
                    TestType queue;

                    WHEN("A value is enqueued via the const lvalue overload") {
                        const int value = 7;
                        queue.enqueue(value);

                        THEN("The same value is dequeued") {
                            int out = 0;
                            CHECK(queue.try_dequeue(out));
                            CHECK(out == 7);
                            assert_queue_reports_empty(queue);
                        }
                    }
                }
            }

            TEMPLATE_TEST_CASE("Scenario: A queue used by a single producer and single consumer preserves FIFO order",
                               "[threading][queue]",
                               MPSCQueue<int>,
                               TaskQueue<int>) {
                GIVEN("An empty queue") {
                    TestType queue;

                    WHEN("Two values are enqueued in order") {
                        queue.enqueue(1);
                        queue.enqueue(2);

                        THEN("They are dequeued in the same order and the queue is then empty") {
                            int value = 0;
                            CHECK(queue.try_dequeue(value));
                            CHECK(value == 1);
                            CHECK(queue.try_dequeue(value));
                            CHECK(value == 2);
                            assert_queue_reports_empty(queue);
                        }
                    }
                }
            }

            TEMPLATE_TEST_CASE("Scenario: A queue can store move-only payloads",
                               "[threading][queue]",
                               MPSCQueue<std::unique_ptr<int>>,
                               TaskQueue<std::unique_ptr<int>>) {
                GIVEN("A queue of std::unique_ptr<int>") {
                    TestType queue;

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

            TEMPLATE_TEST_CASE("Scenario: A queue handles many enqueues from one thread followed by many dequeues",
                               "[threading][queue]",
                               MPSCQueue<int>,
                               TaskQueue<int>) {
                GIVEN("A queue with many sequentially enqueued integers") {
                    TestType queue;
                    constexpr std::size_t many_blocks = 78;
                    constexpr std::size_t many_extra  = 8;
                    constexpr std::size_t item_count  = many_blocks * TestType::BLOCK_SIZE + many_extra;
                    for (std::size_t i = 0; i < item_count; ++i) {
                        queue.enqueue(static_cast<int>(i));
                    }

                    WHEN("They are all dequeued in turn") {
                        bool sequence_holds = true;
                        for (std::size_t i = 0; i < item_count; ++i) {
                            int value = -1;
                            if (!queue.try_dequeue(value) || value != static_cast<int>(i)) {
                                sequence_holds = false;
                                break;
                            }
                        }

                        THEN("Each dequeue returns the next integer in order and the queue is empty") {
                            CHECK(sequence_holds);
                            assert_queue_reports_empty(queue);
                        }
                    }
                }
            }

            TEMPLATE_TEST_CASE("Scenario: A queue consumer waits while a producer links the next block",
                               "[threading][queue]",
                               MPSCQueue<int>,
                               TaskQueue<int>) {
                GIVEN("A queue with one full block and a producer about to overflow it") {
                    TestType queue;
                    for (std::size_t i = 0; i < TestType::BLOCK_SIZE; ++i) {
                        queue.enqueue(static_cast<int>(i));
                    }

                    WHEN("A producer and consumer race across the block boundary") {
                        std::atomic<bool> producer_done{false};
                        std::thread producer([&] {
                            for (std::size_t i = TestType::BLOCK_SIZE; i < 2 * TestType::BLOCK_SIZE; ++i) {
                                queue.enqueue(static_cast<int>(i));
                            }
                            producer_done.store(true, std::memory_order_release);
                        });

                        bool in_order = true;
                        for (std::size_t expected = 0; expected < 2 * TestType::BLOCK_SIZE; ++expected) {
                            int value = -1;
                            while (!queue.try_dequeue(value)) {
                                std::this_thread::yield();
                            }
                            if (value != static_cast<int>(expected)) {
                                in_order = false;
                                break;
                            }
                        }

                        producer.join();

                        THEN("Every integer is delivered in order despite the block rollover race") {
                            CHECK(producer_done.load(std::memory_order_acquire));
                            CHECK(in_order);
                            assert_queue_reports_empty(queue);
                        }
                    }
                }
            }

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
