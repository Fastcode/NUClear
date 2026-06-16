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
#ifndef NUCLEAR_THREADING_SCHEDULER_QUEUE_QUEUE_HPP
#define NUCLEAR_THREADING_SCHEDULER_QUEUE_QUEUE_HPP

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            /**
             * Abstract interface used by Pool so a single bucket array can hold either an
             * MPMC TaskQueue (for multi-consumer pools) or an MPSCQueue (for single-consumer
             * pools such as MainThread or Trace).
             *
             * The per-call indirection cost is negligible compared to the atomic ops inside
             * the concrete enqueue/dequeue implementations, and the simpler MPSC queue is a
             * meaningful win for pools that are by construction single-consumer.
             *
             * @tparam T the element type stored in the queue
             */
            template <typename T>
            class Queue {
            public:
                Queue()                        = default;
                Queue(const Queue&)            = delete;
                Queue(Queue&&)                 = delete;
                Queue& operator=(const Queue&) = delete;
                Queue& operator=(Queue&&)      = delete;
                virtual ~Queue()               = default;

                /**
                 * Push an item into the queue.
                 *
                 * Must be safe to call from any thread concurrently with other enqueue and dequeue operations.
                 *
                 * @param item the value to enqueue (moved into place)
                 */
                virtual void enqueue(T&& item) = 0;

                /**
                 * Try to pop one item from the queue without blocking.
                 *
                 * @param out receives the dequeued value when this returns true
                 *
                 * @return true if `out` was populated; false if the queue was empty
                 */
                virtual bool try_dequeue(T& out) = 0;
            };

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_QUEUE_QUEUE_HPP
