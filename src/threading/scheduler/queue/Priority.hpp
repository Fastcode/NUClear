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

#include <cstddef>
#include <cstdint>

#include "../../../PriorityLevel.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {
        namespace queue {

            /// Fixed scheduler priority buckets (REALTIME, HIGH, NORMAL, LOW, IDLE).
            enum class PriorityBucket : std::uint8_t {
                REALTIME = 0,
                HIGH     = 1,
                NORMAL   = 2,
                LOW      = 3,
                IDLE     = 4,
            };

            /// Number of priority buckets.
            static constexpr std::size_t PRIORITY_BUCKETS = static_cast<std::size_t>(PriorityBucket::IDLE) + 1;

            /**
             * Map a DSL priority level to a fixed scheduler bucket.
             *
             * Seven DSL levels (UNKNOWN, IDLE, LOWEST, LOW, NORMAL, HIGH, HIGHEST, REALTIME) collapse into five
             * buckets: LOWEST and HIGHEST share adjacent buckets; UNKNOWN maps to IDLE.
             *
             * Lower bucket indices are higher runtime priority so buckets can be scanned from 0 upward.
             *
             * @param level the task priority level
             *
             * @return the fixed priority bucket
             */
            inline PriorityBucket priority_bucket(const PriorityLevel& level) {
                switch (level()) {
                    case PriorityLevel::REALTIME: return PriorityBucket::REALTIME;
                    case PriorityLevel::HIGHEST:
                    case PriorityLevel::HIGH: return PriorityBucket::HIGH;
                    case PriorityLevel::NORMAL: return PriorityBucket::NORMAL;
                    case PriorityLevel::LOW:
                    case PriorityLevel::LOWEST: return PriorityBucket::LOW;
                    case PriorityLevel::IDLE:
                    case PriorityLevel::UNKNOWN:
                    default: return PriorityBucket::IDLE;
                }
            }

            /**
             * Map a task priority level to a bucket index.
             *
             * @param level the task priority level
             *
             * @return bucket index in [0, PRIORITY_BUCKETS)
             */
            inline std::size_t priority_index(const PriorityLevel& level) {
                return static_cast<std::size_t>(priority_bucket(level));
            }

            /**
             * Return the higher of two priority levels.
             *
             * @param a first priority level
             * @param b second priority level
             *
             * @return the higher priority level
             */
            constexpr PriorityLevel max_priority(const PriorityLevel& a, const PriorityLevel& b) {
                return a > b ? a : b;
            }

            /**
             * Return the next lower DSL priority level, clamped at IDLE.
             *
             * @param level the current priority level
             *
             * @return the lowered priority level
             */
            constexpr PriorityLevel lower_priority(const PriorityLevel& level) {
                switch (level()) {
                    case PriorityLevel::REALTIME: return PriorityLevel::HIGHEST;
                    case PriorityLevel::HIGHEST: return PriorityLevel::HIGH;
                    case PriorityLevel::HIGH: return PriorityLevel::NORMAL;
                    case PriorityLevel::NORMAL: return PriorityLevel::LOW;
                    case PriorityLevel::LOW: return PriorityLevel::LOWEST;
                    case PriorityLevel::LOWEST:
                    case PriorityLevel::IDLE:
                    case PriorityLevel::UNKNOWN:
                    default: return PriorityLevel::IDLE;
                }
            }

        }  // namespace queue
    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_QUEUE_PRIORITY_HPP
