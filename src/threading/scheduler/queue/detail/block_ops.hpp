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
#ifndef NUCLEAR_THREADING_SCHEDULER_QUEUE_DETAIL_BLOCK_OPS_HPP
#define NUCLEAR_THREADING_SCHEDULER_QUEUE_DETAIL_BLOCK_OPS_HPP

#include <atomic>
#include <memory>
#include <thread>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#elif defined(_MSC_VER) && (defined(__aarch64__) || defined(_M_ARM64))
#include <intrin.h>
#endif

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {
            namespace detail {

                /**
                 * Shared lock-free block-management helpers used by both the MPSC and MPMC block queues.
                 *
                 * These routines implement the parts of the block-list infrastructure that are identical
                 * (and identically safe) regardless of the producer/consumer cardinality: allocating a
                 * block, retiring a drained block onto the graveyard, and linking the next block into the
                 * list. They are deliberately templated free functions so they inline to exactly the same
                 * machine code the queues previously emitted inline, preserving the TSAN-validated memory
                 * ordering verbatim. The MPSC-vs-MPMC differences (Block layout, liveness model, consumer
                 * logic) intentionally remain in the individual queue headers.
                 *
                 * Each Block type is required to expose:
                 *   - std::atomic<Block*> next;
                 *   - Block*              graveyard_next;
                 */

                inline void cpu_pause() {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
                    _mm_pause();
#elif defined(__aarch64__) || defined(_M_ARM64)
#if defined(_MSC_VER)
                    __yield();
#else
                    __asm__ __volatile__("yield" ::: "memory");
#endif
#endif
                }

                /// Brief CPU pause burst then one scheduler yield (one backoff step per caller iteration).
                inline void pause_and_yield() {
                    for (int spin = 0; spin < 64; ++spin) {
                        cpu_pause();
                    }
                    std::this_thread::yield();
                }

                /// Spin with a brief CPU pause, then yield, until `pred()` is true.
                template <typename Pred>
                void spin_until(const Pred& pred) {
                    for (int spin = 0; spin < 64 && !pred(); ++spin) {
                        cpu_pause();
                    }
                    while (!pred()) {
                        std::this_thread::yield();
                    }
                }

                /**
                 * Allocate a fresh block for the queue's block list.
                 *
                 * @tparam Block the queue block type (must expose `next` and `graveyard_next`)
                 *
                 * @return a default-constructed heap-allocated block
                 */
                template <typename Block>
                Block* allocate_block() {
                    return new Block();
                }

                /**
                 * Retire a fully drained block onto the graveyard list for deferred deletion.
                 *
                 * Producers can still be operating on a block after the consumer advances head past it
                 * (e.g. a producer that loaded the tail before it advanced). To avoid use-after-free we
                 * never delete blocks while the queue is live; they are kept on a graveyard list and freed
                 * in the destructor. In steady state the graveyard length is bounded by the peak number of
                 * in-flight blocks.
                 *
                 * @tparam Block the queue block type
                 *
                 * @param graveyard atomic head of the graveyard list
                 * @param block     the block to retire (must not contain live payloads)
                 */
                template <typename Block>
                void retire_block(std::atomic<Block*>& graveyard, Block* block) {
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

                /**
                 * Attempt to link a newly allocated successor block onto a full block.
                 *
                 * @tparam Block the queue block type
                 *
                 * @param block the full block whose `next` should be linked
                 *
                 * @return true if this caller linked the new block; false if another producer linked first
                 */
                template <typename Block>
                bool link_next_block(Block* block) {
                    // Hold the new block in a unique_ptr so that if the CAS fails (another producer
                    // linked the next block first) we don't leak the freshly allocated Block.
                    // Function arguments are unconditionally evaluated in C++, so the previous form
                    // `compare_exchange_strong(expected, allocate_block(), ...)` leaked one Block per
                    // contended overflow.
                    Block* expected = nullptr;
                    std::unique_ptr<Block> candidate(allocate_block<Block>());
                    if (block->next.compare_exchange_strong(expected,
                                                            candidate.get(),
                                                            std::memory_order_acq_rel)) {
                        candidate.release();
                        return true;
                    }
                    return expected != nullptr;
                }

            }  // namespace detail
        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_QUEUE_DETAIL_BLOCK_OPS_HPP
