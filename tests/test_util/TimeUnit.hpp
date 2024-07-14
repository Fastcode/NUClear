/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#ifndef TEST_UTIL_TIME_UNIT_HPP
#define TEST_UTIL_TIME_UNIT_HPP

#ifndef NUCLEAR_TEST_TIME_UNIT_NUM
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define NUCLEAR_TEST_TIME_UNIT_NUM 1
#endif
#ifndef NUCLEAR_TEST_TIME_UNIT_DEN
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
    #define NUCLEAR_TEST_TIME_UNIT_DEN 20
#endif

#include <chrono>

namespace test_util {

/**
 * Units that time based tests should use to measure time.
 * This can be changed so that slower systems (such as CI) can run the tests with a larger time unit since they can be
 * slow and fail.
 * To change from the default define NUCLEAR_TEST_TIME_UNIT_NUM and NUCLEAR_TEST_TIME_UNIT_DEN to the desired time unit.
 */
using TimeUnit = std::chrono::duration<int64_t, std::ratio<NUCLEAR_TEST_TIME_UNIT_NUM, NUCLEAR_TEST_TIME_UNIT_DEN>>;

/**
 * Rounds the given duration to the nearest TimeUnit.
 *
 * @tparam T The type of the duration.
 *
 * @param duration The duration to be rounded.
 *
 * @return The rounded duration in TimeUnit.
 */
template <typename T>
TimeUnit round_to_test_units(const T& duration) {
    const double d = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
    const double t = (TimeUnit::period::den * d) / TimeUnit::period::num;
    return TimeUnit(std::lround(t));
}

}  // namespace test_util

#endif  // TEST_UTIL_TIME_UNIT_HPP
