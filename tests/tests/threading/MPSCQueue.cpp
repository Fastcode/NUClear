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

#include "test_util/queue_bdd_helpers.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            SCENARIO("An MPSCQueue destroyed while non-empty runs the destructors of its remaining items",
                     "[threading][queue][MPSCQueue]") {
                test_util::queue_bdd::destructor_runs_remaining_destructors_scenario<MPSCQueue<test_util::QueueLiveTracker>>();
            }

            SCENARIO("An MPSCQueue accepts copy-enqueued const payloads", "[threading][queue][MPSCQueue]") {
                test_util::queue_bdd::copy_enqueue_const_payload_scenario<MPSCQueue<int>>();
            }

            SCENARIO("An MPSCQueue consumer waits while a producer links the next block",
                     "[threading][queue][MPSCQueue]") {
                test_util::queue_bdd::block_boundary_producer_consumer_race_scenario<MPSCQueue<int>>();
            }

            SCENARIO("An MPSCQueue used by a single producer and single consumer preserves FIFO order",
                     "[threading][queue][MPSCQueue]") {
                test_util::queue_bdd::single_producer_consumer_fifo_scenario<MPSCQueue<int>>();
            }

            SCENARIO("An MPSCQueue can store move-only payloads", "[threading][queue][MPSCQueue]") {
                test_util::queue_bdd::move_only_payload_scenario<MPSCQueue<std::unique_ptr<int>>>();
            }

            SCENARIO("An MPSCQueue handles many enqueues from one thread followed by many dequeues",
                     "[threading][queue][MPSCQueue]") {
                test_util::queue_bdd::sequential_enqueue_dequeue_scenario<MPSCQueue<int>>();
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
