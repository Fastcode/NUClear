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

#ifndef TEST_UTIL_QUEUE_LIVE_TRACKER_HPP
#define TEST_UTIL_QUEUE_LIVE_TRACKER_HPP

#include <atomic>

namespace test_util {

/// Counts how many LiveTracker instances are currently alive so queue tests can detect skipped destructors.
inline std::atomic<int>& queue_live_tracker_count() {
    static std::atomic<int> count{0};  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    return count;
}

/// Construction (incl. copy/move) increments; destruction decrements.
struct QueueLiveTracker {
    int value;
    explicit QueueLiveTracker(int v = 0) : value(v) {
        queue_live_tracker_count().fetch_add(1, std::memory_order_relaxed);
    }
    QueueLiveTracker(const QueueLiveTracker& other) : value(other.value) {
        queue_live_tracker_count().fetch_add(1, std::memory_order_relaxed);
    }
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
