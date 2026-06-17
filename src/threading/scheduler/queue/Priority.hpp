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
#ifndef NUCLEAR_THREADING_SCHEDULER_QUEUE_PRIORITY_HPP
#define NUCLEAR_THREADING_SCHEDULER_QUEUE_PRIORITY_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            /// Fixed scheduler priority buckets (REALTIME, HIGH, NORMAL, LOW, IDLE).
            enum class PriorityLevel : std::uint8_t {
                REALTIME = 0,
                HIGH     = 1,
                NORMAL   = 2,
                LOW      = 3,
                IDLE     = 4,
            };

            /// Number of priority buckets.
            static constexpr std::size_t PRIORITY_BUCKETS = static_cast<std::size_t>(PriorityLevel::IDLE) + 1;

            /**
             * Map a priority level to a bucket index.
             *
             * Lower indices are higher runtime priority so buckets can be scanned from 0 upward.
             *
             * @param level the task priority level
             *
             * @return bucket index in [0, PRIORITY_BUCKETS)
             */
            inline std::size_t priority_index(const PriorityLevel level) {
                return static_cast<std::size_t>(level);
            }

            /**
             * Return the higher of two priority levels.
             *
             * @param a first priority level
             * @param b second priority level
             *
             * @return the higher priority level
             */
            inline constexpr PriorityLevel max_priority(const PriorityLevel a, const PriorityLevel b) {
                return a < b ? a : b;
            }

            /**
             * Return the next lower priority level, clamped at IDLE.
             *
             * @param level the current priority level
             *
             * @return the lowered priority level
             */
            inline constexpr PriorityLevel lower_priority(const PriorityLevel level) {
                switch (level) {
                    case PriorityLevel::REALTIME: return PriorityLevel::HIGH;
                    case PriorityLevel::HIGH: return PriorityLevel::NORMAL;
                    case PriorityLevel::NORMAL: return PriorityLevel::LOW;
                    case PriorityLevel::LOW: return PriorityLevel::IDLE;
                    case PriorityLevel::IDLE: return PriorityLevel::IDLE;
                }
                return PriorityLevel::IDLE;
            }

        }  // namespace queue
    }  // namespace scheduler

    /// Canonical task priority type used by reactions and the scheduler.
    using PriorityLevel = scheduler::queue::PriorityLevel;

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_QUEUE_PRIORITY_HPP
