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

#ifndef NUCLEAR_MESSAGE_TIME_TRAVEL_HPP
#define NUCLEAR_MESSAGE_TIME_TRAVEL_HPP

#include "../clock.hpp"

namespace NUClear {
namespace message {

    /**
     * @brief This message is used to adjust the time of the system clock and the rate at which time passes.
     *
     * Using this message allows the NUClear system to adapt to the change by adjusting any time based operations
     * to the new time and rate.
     */
    struct TimeTravel {
        /// @brief The amount of time to adjust the system clock by
        clock::duration adjustment{0};
        /// @brief The rate at which time should pass
        double rtf = 1.0;

        TimeTravel() = default;
        TimeTravel(const clock::duration& adjustment, double rtf = 1.0) : adjustment(adjustment), rtf(rtf) {}
    };

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_TIME_TRAVEL_HPP
