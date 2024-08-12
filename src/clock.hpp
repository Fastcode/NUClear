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
struct clock : NUCLEAR_CLOCK_TYPE {
    using base_clock = NUCLEAR_CLOCK_TYPE;

    /**
     * Get the current time of the clock.
     *
     * @return The current time of the clock.
     */
    static time_point now();

    /**
     * Adjust the clock by a specified duration and real-time factor.
     *
     * @param adjustment The duration by which to adjust the clock.
     * @param rtf        The real-time factor to apply to the clock.
     */
    static void adjust_clock(const duration& adjustment, const double& rtf = 1.0);

    /**
     * Set the clock to a specified time and real-time factor.
     *
     * @param time The time to set the clock to.
     * @param rtf  The real-time factor to apply to the clock.
     */
    static void set_clock(const time_point& time, const double& rtf = 1.0);


    /**
     * Get the real-time factor of the clock.
     *
     * @return The real-time factor of the clock
     */
    static double rtf();

private:
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

}  // namespace NUClear

#endif  // NUCLEAR_CLOCK_HPP
