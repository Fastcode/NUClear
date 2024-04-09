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

namespace NUClear {

std::mutex clock::mutex;                       // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::array<clock::ClockData, 3> clock::data =  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    std::array<clock::ClockData, 3>{};         // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<int> clock::active{0};             // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

clock::time_point clock::now() {
    const ClockData current = data[active.load()];  // Take a copy in case it changes
    return current.epoch + dc((base_clock::now() - current.base_from) * current.rtf);
}

void clock::adjust_clock(const duration& adjustment, const double& rtf) {
    const std::lock_guard<std::mutex> lock(mutex);
    // Load the current state
    const auto& current = data[active.load()];
    const int n         = static_cast<int>((active.load() + 1) % data.size());
    auto& next          = data[n];

    // Perform the update
    auto base      = base_clock::now();
    next.epoch     = current.epoch + adjustment + dc((base - current.base_from) * current.rtf);
    next.base_from = base;
    next.rtf       = rtf;
    active         = n;
}

void clock::set_clock(const time_point& time, const double& rtf) {
    const std::lock_guard<std::mutex> lock(mutex);
    // Load the current state
    const int n = static_cast<int>((active.load() + 1) % data.size());
    auto& next  = data[n];

    // Perform the update
    auto base      = base_clock::now();
    next.epoch     = time;
    next.base_from = base;
    next.rtf       = rtf;
    active         = n;
}

}  // namespace NUClear
