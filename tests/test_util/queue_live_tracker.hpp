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

#ifndef TEST_UTIL_QUEUE_LIVE_TRACKER_HPP
#define TEST_UTIL_QUEUE_LIVE_TRACKER_HPP

#include <atomic>

namespace test_util {

/**
 * Global live-instance counter used by queue destruction tests.
 *
 * @return reference to the process-wide count of live QueueLiveTracker objects
 */
inline std::atomic<int>& queue_live_tracker_count() {
    static std::atomic<int> count{0};  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    return count;
}

/**
 * Test payload whose constructor and destructor update queue_live_tracker_count().
 *
 * Construction (including copy and move) increments the counter; destruction decrements it.
 */
struct QueueLiveTracker {
    /// Stored integer payload inspected by tests after dequeue.
    int value;

    /**
     * @param v the stored integer payload
     */
    explicit QueueLiveTracker(int v = 0) : value(v) {
        queue_live_tracker_count().fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @param other the tracker to copy the payload from
     */
    QueueLiveTracker(const QueueLiveTracker& other) : value(other.value) {
        queue_live_tracker_count().fetch_add(1, std::memory_order_relaxed);
    }

    /**
     * @param other the tracker to move the payload from
     */
    QueueLiveTracker(QueueLiveTracker&& other) noexcept : value(other.value) {
        queue_live_tracker_count().fetch_add(1, std::memory_order_relaxed);
    }

    QueueLiveTracker& operator=(const QueueLiveTracker&) = default;
    QueueLiveTracker& operator=(QueueLiveTracker&&) noexcept = default;

    ~QueueLiveTracker() {
        queue_live_tracker_count().fetch_sub(1, std::memory_order_relaxed);
    }
};

}  // namespace test_util

#endif  // TEST_UTIL_QUEUE_LIVE_TRACKER_HPP
