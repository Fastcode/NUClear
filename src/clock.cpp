/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
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
#include "clock.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <mutex>

namespace NUClear {

namespace {
    template <typename T>
    clock::duration dc(const T& t) {
        return std::chrono::duration_cast<clock::duration>(t);
    }
}  // namespace

clock::time_point clock::now() {
    const ClockData current = data[active.load(std::memory_order_acquire)];  // Take a copy in case it changes
    return current.epoch + dc((base_clock::now() - current.base_from) * current.rtf);
}

void clock::adjust_clock(const duration& adjustment, const double& rtf) {
    const std::lock_guard<std::mutex> lock(mutex);
    // Load the current state
    const int c         = active.load(std::memory_order_relaxed);
    const auto& current = data[c];
    const int n         = int((c + 1) % data.size());
    auto& next          = data[n];

    // Perform the update
    auto base      = base_clock::now();
    next.epoch     = current.epoch + adjustment + dc((base - current.base_from) * current.rtf);
    next.base_from = base;
    next.rtf       = rtf;
    active.store(n, std::memory_order_release);
}

void clock::set_clock(const time_point& time, const double& rtf) {
    const std::lock_guard<std::mutex> lock(mutex);
    // Load the next state
    const int c = active.load(std::memory_order_acquire);
    const int n = int((c + 1) % data.size());
    auto& next  = data[n];

    // Perform the update
    auto base      = base_clock::now();
    next.epoch     = time;
    next.base_from = base;
    next.rtf       = rtf;
    active         = n;
}

double clock::rtf() {
    return data[active.load(std::memory_order_acquire)].rtf;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::mutex clock::mutex;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::array<clock::ClockData, 3> clock::data = std::array<typename clock::ClockData, 3>{};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<int> clock::active{0};

}  // namespace NUClear
