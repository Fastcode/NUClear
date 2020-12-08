/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include <chrono>

namespace NUClear {

#ifndef NUCLEAR_CLOCK_TYPE
#    define NUCLEAR_CLOCK_TYPE std::chrono::steady_clock
#endif  // NUCLEAR_CLOCK_TYPE

/// @brief The base clock that is used when defining the NUClear clock
using base_clock = NUCLEAR_CLOCK_TYPE;

#ifndef NUCLEAR_CUSTOM_CLOCK

/// @brief The clock that is used throughout the entire nuclear system
using clock = base_clock;

#else

struct clock {
    using rep                       = base_clock::rep;
    using period                    = base_clock::period;
    using duration                  = base_clock::duration;
    using time_point                = base_clock::time_point;
    static constexpr bool is_steady = false;

    static time_point now();
};

#endif

}  // namespace NUClear

#endif  // NUCLEAR_CLOCK_HPP
