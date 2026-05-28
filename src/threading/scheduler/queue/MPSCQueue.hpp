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
#ifndef NUCLEAR_THREADING_SCHEDULER_QUEUE_MPSC_QUEUE_HPP
#define NUCLEAR_THREADING_SCHEDULER_QUEUE_MPSC_QUEUE_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>

#include "Queue.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            /**
             * Lock-free multi-producer single-consumer unbounded FIFO queue.
             *
             * The producer side is identical to the MPMC TaskQueue (block-based, atomic
             * fetch_add to claim a slot). The consumer side is simpler because there is
             * by contract only ever one consumer thread: the per-block read counter is a
             * plain integer, no CAS is needed to claim a slot, and the consumer can delete
             * fully-drained blocks immediately (subject to letting concurrent producers
             * finish touching them, handled via a graveyard like the MPMC variant).
             *
             * Use this in pools that are declared with `concurrency = 1` (e.g. MainThread,
             * the TraceController pool, or any user pool with a single worker thread).
             */
            template <typename T>
            class MPSCQueue : public Queue<T> {
                static_assert(std::is_move_constructible<T>::value, "MPSCQueue requires move constructible T");

            private:
                static constexpr std::size_t BLOCK_SIZE = 64;

                struct Slot {
                    std::atomic<bool> committed{false};
                    /// Raw aligned storage for the T payload. Left value-initialised (zeroed) so the
                    /// constructor fully covers all members; placement-new overwrites it on enqueue.
                    alignas(T) std::array<unsigned char, sizeof(T)> storage{};
                };

                struct Block {
                    std::array<Slot, BLOCK_SIZE> slots{};
                    /// Producer claim counter, fetched by every enqueuer (atomic, MP-safe).
                    std::atomic<std::size_t> write{0};
                    /// Consumer read counter, only touched by the single consumer (non-atomic).
                    std::size_t read{0};
                    std::atomic<Block*> next{nullptr};
                    Block* graveyard_next{nullptr};
                };

                static T* slot_ptr(Slot& slot) {
                    return reinterpret_cast<T*>(slot.storage.data());
                }

                static Block* allocate_block() {
                    return new Block();
                }

                // Producers can still be operating on a block after the consumer advances head past
                // it (e.g. a producer that loaded tail_block before it advanced is in
                // link_next_block). To avoid use-after-free we never delete blocks while the queue
                // is live; they are kept on a graveyard list and freed in the destructor. In steady
                // state the graveyard length is bounded by the peak number of in-flight blocks.
                void retire_block(Block* block) {
                    Block* head_graveyard = graveyard.load(std::memory_order_acquire);
                    while (true) {
                        block->graveyard_next = head_graveyard;
                        if (graveyard.compare_exchange_weak(head_graveyard,
                                                            block,
                                                            std::memory_order_release,
                                                            std::memory_order_relaxed)) {
                            return;
                        }
                    }
                }

                bool link_next_block(Block* block) {
                    // Hold the new block in a unique_ptr so that if the CAS fails (another producer
                    // linked the next block first) we don't leak the freshly allocated Block.
                    // Function arguments are unconditionally evaluated in C++, so the previous form
                    // `compare_exchange_strong(expected, allocate_block(), ...)` leaked one Block per
                    // contended overflow.
                    Block* expected = nullptr;
                    std::unique_ptr<Block> candidate(allocate_block());
                    if (block->next.compare_exchange_strong(expected,
                                                            candidate.get(),
                                                            std::memory_order_acq_rel)) {
                        candidate.release();
                        return true;
                    }
                    return expected != nullptr;
                }

                void advance_tail(Block* expected, Block* next) {
                    Block* tail_ptr = tail_block.load(std::memory_order_acquire);
                    while (tail_ptr == expected) {
                        if (tail_block.compare_exchange_weak(tail_ptr,
                                                             next,
                                                             std::memory_order_release,
                                                             std::memory_order_relaxed)) {
                            return;
                        }
                    }
                }

                /// Consumer-owned head pointer. Non-atomic because only the consumer reads/writes it.
                Block* head_block;
                /// Producer-shared tail pointer. Atomic because any number of producers chase it.
                std::atomic<Block*> tail_block;
                /// Linked list of retired blocks that are kept alive until the queue is destroyed.
                std::atomic<Block*> graveyard;

            public:
                MPSCQueue() {
                    auto* initial = new Block();
                    head_block    = initial;
                    tail_block.store(initial, std::memory_order_relaxed);
                    graveyard.store(nullptr, std::memory_order_relaxed);
                }

                MPSCQueue(const MPSCQueue&)            = delete;
                MPSCQueue& operator=(const MPSCQueue&) = delete;
                MPSCQueue(MPSCQueue&&)                 = delete;
                MPSCQueue& operator=(MPSCQueue&&)      = delete;

                ~MPSCQueue() override {
                    Block* current = head_block;
                    while (current != nullptr) {
                        Block* next = current->next.load(std::memory_order_relaxed);
                        delete current;
                        current = next;
                    }

                    Block* dead = graveyard.load(std::memory_order_relaxed);
                    while (dead != nullptr) {
                        Block* next = dead->graveyard_next;
                        delete dead;
                        dead = next;
                    }
                }

                void enqueue(const T& item) {
                    T copy(item);
                    enqueue(std::move(copy));
                }

                void enqueue(T&& item) override {
                    while (true) {
                        Block*            block = tail_block.load(std::memory_order_acquire);
                        const std::size_t index = block->write.fetch_add(1, std::memory_order_relaxed);

                        if (index < BLOCK_SIZE) {
                            Slot& slot = block->slots[index];
                            new (slot.storage.data()) T(std::move(item));
                            slot.committed.store(true, std::memory_order_release);
                            return;
                        }

                        // Block full. Link the next one (or help an in-flight linker) and advance tail.
                        link_next_block(block);

                        Block* next = block->next.load(std::memory_order_acquire);
                        advance_tail(block, next);
                    }
                }

                bool try_dequeue(T& out) override {
                    while (true) {
                        const std::size_t write_observed = head_block->write.load(std::memory_order_acquire);
                        const std::size_t published      = std::min(write_observed, static_cast<std::size_t>(BLOCK_SIZE));

                        if (head_block->read < published) {
                            Slot& slot = head_block->slots[head_block->read];
                            // Producer's claim happens-before its commit, but commit may not be visible
                            // yet if we raced it. Spin briefly until the data is published.
                            while (!slot.committed.load(std::memory_order_acquire)) {
                                std::this_thread::yield();
                            }

                            out = std::move(*slot_ptr(slot));
                            slot_ptr(slot)->~T();
                            ++head_block->read;
                            return true;
                        }

                        // Block drained from this consumer's perspective. Try to move to the next.
                        Block* next = head_block->next.load(std::memory_order_acquire);
                        if (next == nullptr) {
                            // If a producer has already overflowed past BLOCK_SIZE we know they're
                            // mid-way through linking the next block; wait briefly for it to appear.
                            if (write_observed > BLOCK_SIZE) {
                                std::this_thread::yield();
                                continue;
                            }
                            return false;
                        }

                        // We're the sole consumer so advancing head_block is a plain store. The old
                        // block goes to the graveyard so any producer that still holds a pointer to
                        // it (e.g. one mid-way through link_next_block) doesn't touch freed memory.
                        Block* old = head_block;
                        head_block = next;
                        retire_block(old);
                    }
                }
            };

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_QUEUE_MPSC_QUEUE_HPP
