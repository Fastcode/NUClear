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

#ifndef NUCLEAR_CLOCK_HPP
#define NUCLEAR_CLOCK_HPP

// Default to using the system clock but allow it to be overridden by the user
#ifndef NUCLEAR_CLOCK_TYPE
    #define NUCLEAR_CLOCK_TYPE std::chrono::system_clock
#endif  // NUCLEAR_CLOCK_TYPE

#include <array>
#include <atomic>
#include <chrono>
#include <mutex>

namespace NUClear {

struct clock : public NUCLEAR_CLOCK_TYPE {
    using base_clock = NUCLEAR_CLOCK_TYPE;

    static time_point now() {
        ClockData current = data[active.load()];  // Take a copy in case it changes
        return current.epoch + dc((base_clock::now() - current.base_from) * current.rtf);
    }

    static void adjust_clock(const duration& adjustment, const double& rtf = 1.0) {
        std::lock_guard<std::mutex> lock(mutex);
        // Load the current state
        auto& current = data[active.load()];
        int n         = (active.load() + 1) % data.size();
        auto& next    = data[n];

        // Perform the update
        auto base      = base_clock::now();
        next.epoch     = current.epoch + adjustment + dc((base - current.base_from) * current.rtf);
        next.base_from = base;
        next.rtf       = rtf;
        active         = n;
    }

    static void set_clock(const time_point& time, const double& rtf = 1.0) {
        std::lock_guard<std::mutex> lock(mutex);
        // Load the current state
        int n      = (active.load() + 1) % data.size();
        auto& next = data[n];

        // Perform the update
        auto base      = base_clock::now();
        next.epoch     = time;
        next.base_from = base;
        next.rtf       = rtf;
        active         = n;
    }

private:
    template <typename T>
    duration static dc(const T& t) {
        return std::chrono::duration_cast<duration>(t);
    }

    struct ClockData {
        /// When the clock was last updated under the true time
        time_point base_from;
        /// Our calculated time when the clock was last updated in simulated time
        time_point epoch = base_from;
        /// The real time factor of the simulated clock
        double rtf;

        ClockData() : base_from(base_clock::now()), epoch(base_from), rtf(1.0) {}
    };

    static std::mutex mutex;               // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    static std::array<ClockData, 3> data;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
    static std::atomic<int> active;        // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
};

std::mutex clock::mutex;
std::array<clock::ClockData, 3> clock::data = std::array<clock::ClockData, 3>{};
std::atomic<int> clock::active{0};


}  // namespace NUClear

#endif  // NUCLEAR_CLOCK_HPP
