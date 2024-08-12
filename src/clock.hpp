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

/**
 * A clock class that extends a base clock type and allows for clock adjustment and setting.
 */
template <typename = void>
struct nuclear_clock : public NUCLEAR_CLOCK_TYPE {
    using base_clock = NUCLEAR_CLOCK_TYPE;

    /**
     * Get the current time of the clock.
     *
     * @return The current time of the clock.
     */
    static time_point now() {
        const ClockData current = data[active.load()];  // Take a copy in case it changes
        return current.epoch + dc((base_clock::now() - current.base_from) * current.rtf);
    }

    /**
     * Adjust the clock by a specified duration and real-time factor.
     *
     * @param adjustment The duration by which to adjust the clock.
     * @param rtf        The real-time factor to apply to the clock.
     */
    static void adjust_clock(const duration& adjustment, const double& rtf = 1.0) {
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

    /**
     * Set the clock to a specified time and real-time factor.
     *
     * @param time The time to set the clock to.
     * @param rtf  The real-time factor to apply to the clock.
     */
    static void set_clock(const time_point& time, const double& rtf = 1.0) {
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


    /**
     * Get the real-time factor of the clock.
     *
     * @return The real-time factor of the clock.
     */
    static double rtf() {
        return data[active.load()].rtf;
    }

private:
    /**
     * Convert a duration to the clock's duration type.
     *
     * @tparam T The type of the duration.
     *
     * @param t The duration to convert.
     *
     * @return  The converted duration.
     */
    template <typename T>
    duration static dc(const T& t) {
        return std::chrono::duration_cast<duration>(t);
    }

    /**
     * Data structure to hold clock information.
     */
    struct ClockData {
        /// When the clock was last updated under the true time
        time_point base_from = base_clock::now();
        /// Our calculated time when the clock was last updated in simulated time
        time_point epoch = base_from;
        /// The real time factor of the simulated clock
        double rtf = 1.0;

        ClockData() = default;
    };

    /// The mutex to protect the clock data
    static std::mutex mutex;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    /// The clock data for the system
    static std::array<ClockData, 3> data;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    /// The active clock data index
    static std::atomic<int> active;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
};

template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::mutex nuclear_clock<T>::mutex;
template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::array<typename nuclear_clock<T>::ClockData, 3> nuclear_clock<T>::data =
    std::array<typename nuclear_clock<T>::ClockData, 3>{};
template <typename T>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::atomic<int> nuclear_clock<T>::active{0};

using clock = nuclear_clock<>;


}  // namespace NUClear

#endif  // NUCLEAR_CLOCK_HPP
