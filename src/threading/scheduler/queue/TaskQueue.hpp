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
#ifndef NUCLEAR_THREADING_SCHEDULER_QUEUE_TASK_QUEUE_HPP
#define NUCLEAR_THREADING_SCHEDULER_QUEUE_TASK_QUEUE_HPP

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
             * Lock-free multi-producer multi-consumer unbounded FIFO queue.
             *
             * Storage is organised in fixed-size blocks linked in a list. Fully drained blocks are
             * retired to a graveyard and deleted when the queue is destroyed. Per-producer FIFO is
             * preserved; cross-producer ordering is not guaranteed.
             */
            template <typename T>
            class TaskQueue : public Queue<T> {
                static_assert(std::is_move_constructible<T>::value, "TaskQueue requires move constructible T");

            private:
                static constexpr std::size_t BLOCK_SIZE = 64;

                struct Block;

                struct Slot {
                    std::atomic<bool> committed{false};
                    /// Raw aligned storage for the T payload. Left value-initialised (zeroed) so the
                    /// constructor fully covers all members; placement-new overwrites it on enqueue.
                    alignas(T) std::array<unsigned char, sizeof(T)> storage{};
                };

                struct Block {
                    std::array<Slot, BLOCK_SIZE> slots{};
                    std::atomic<std::size_t> write{0};
                    std::atomic<std::size_t> read{0};
                    std::atomic<std::size_t> consumed{0};
                    std::atomic<Block*> next{nullptr};
                    Block* graveyard_next{nullptr};
                };

                static T* slot_ptr(Slot& slot) {
                    return reinterpret_cast<T*>(slot.storage.data());
                }

                static void destroy_slot(Slot& slot) {
                    slot_ptr(slot)->~T();
                    slot.committed.store(false, std::memory_order_relaxed);
                }

                // Run ~T on every slot that still holds a live, committed payload. Used by the
                // destructor so a queue torn down while non-empty does not skip the destructors of
                // its remaining elements (e.g. a Task's unique_ptr<ReactionTask>). Only ever called
                // when the queue is quiescent, so the committed flag is a stable per-slot truth.
                static void destroy_live_slots(Block* block) {
                    for (auto& slot : block->slots) {
                        if (slot.committed.load(std::memory_order_relaxed)) {
                            destroy_slot(slot);
                        }
                    }
                }

                static Block* allocate_block() {
                    return new Block();
                }

                // Retired blocks are kept alive on the graveyard so consumers that still hold
                // a stale pointer cannot observe freed memory.
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
                    Block* tail_ptr = tail.load(std::memory_order_acquire);
                    while (tail_ptr == expected) {
                        if (tail.compare_exchange_weak(tail_ptr, next, std::memory_order_release, std::memory_order_relaxed)) {
                            return;
                        }
                    }
                }

                void try_reclaim_block(Block* block) {
                    if (block->consumed.load(std::memory_order_acquire) != BLOCK_SIZE) {
                        return;
                    }

                    Block* head_ptr = head.load(std::memory_order_acquire);
                    if (head_ptr != block) {
                        return;
                    }

                    // Never strand head at nullptr; only advance if a successor block exists.
                    Block* next = block->next.load(std::memory_order_acquire);
                    if (next == nullptr) {
                        return;
                    }
                    if (head.compare_exchange_strong(head_ptr, next, std::memory_order_release, std::memory_order_relaxed)) {
                        retire_block(block);
                    }
                }

                std::atomic<Block*> head;
                std::atomic<Block*> tail;
                std::atomic<Block*> graveyard;

            public:
                TaskQueue() {
                    auto* initial = new Block();
                    head.store(initial, std::memory_order_relaxed);
                    tail.store(initial, std::memory_order_relaxed);
                    graveyard.store(nullptr, std::memory_order_relaxed);
                }

                TaskQueue(const TaskQueue&)            = delete;
                TaskQueue& operator=(const TaskQueue&) = delete;
                TaskQueue(TaskQueue&&)                 = delete;
                TaskQueue& operator=(TaskQueue&&)      = delete;

                ~TaskQueue() override {
                    // Live blocks (reachable from head) may still hold committed-but-undequeued
                    // payloads; destroy those before freeing the storage. Graveyard blocks were
                    // fully drained before retirement, so they hold no live payloads.
                    Block* current = head.load(std::memory_order_relaxed);
                    while (current != nullptr) {
                        Block* next = current->next.load(std::memory_order_relaxed);
                        destroy_live_slots(current);
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
                        Block* block = tail.load(std::memory_order_acquire);
                        const std::size_t index = block->write.fetch_add(1, std::memory_order_relaxed);

                        if (index < BLOCK_SIZE) {
                            Slot& slot = block->slots[index];
                            new (slot.storage.data()) T(std::move(item));
                            slot.committed.store(true, std::memory_order_release);
                            return;
                        }

                        if (!link_next_block(block)) {
                            // Another thread linked next; help advance tail.
                        }

                        Block* next = block->next.load(std::memory_order_acquire);
                        advance_tail(block, next);
                    }
                }

                bool try_dequeue(T& out) override {
                    while (true) {
                        Block* block = head.load(std::memory_order_acquire);

                        const std::size_t published =
                            std::min(block->write.load(std::memory_order_acquire), BLOCK_SIZE);
                        std::size_t read_index = block->read.load(std::memory_order_relaxed);

                        if (read_index >= published) {
                            if (block->consumed.load(std::memory_order_acquire) < published) {
                                // Consumers are still finishing slots in this block; let them progress.
                                std::this_thread::yield();
                            }
                            else {
                                Block* next = block->next.load(std::memory_order_acquire);
                                if (next == nullptr) {
                                    // Producer may still be writing the first slot of an empty-looking block.
                                    if (published == 0 && block->write.load(std::memory_order_acquire) > 0) {
                                        std::this_thread::yield();
                                    }
                                    else {
                                        return false;
                                    }
                                }
                                else if (head.compare_exchange_strong(block,
                                                                      next,
                                                                      std::memory_order_release,
                                                                      std::memory_order_relaxed)) {
                                    // We won the race to advance head past a fully-drained block, so
                                    // we own its retirement. try_reclaim_block() only retires when it
                                    // wins this same head CAS; without retiring here the block would
                                    // be unreachable from both head and the graveyard and thus leak.
                                    retire_block(block);
                                }
                            }
                        }
                        else if (block->read.compare_exchange_weak(read_index,
                                                                   read_index + 1,
                                                                   std::memory_order_acq_rel,
                                                                   std::memory_order_relaxed)) {
                            Slot& slot = block->slots[read_index];
                            while (!slot.committed.load(std::memory_order_acquire)) {
                                std::this_thread::yield();
                            }

                            out = std::move(*slot_ptr(slot));
                            destroy_slot(slot);

                            if (block->consumed.fetch_add(1, std::memory_order_acq_rel) + 1 == BLOCK_SIZE) {
                                try_reclaim_block(block);
                            }

                            return true;
                        }
                    }
                }

                bool empty() const {
                    Block* block = head.load(std::memory_order_acquire);
                    while (block != nullptr) {
                        const std::size_t published =
                            std::min(block->write.load(std::memory_order_acquire), BLOCK_SIZE);
                        if (block->read.load(std::memory_order_relaxed) < published) {
                            return false;
                        }
                        block = block->next.load(std::memory_order_acquire);
                    }
                    return true;
                }
            };

            template <typename T>
            constexpr std::size_t TaskQueue<T>::BLOCK_SIZE;

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_QUEUE_TASK_QUEUE_HPP
