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
     * This message is used to adjust the time of the system clock and the rate at which time passes.
     *
     * Using this message allows the NUClear system to adapt to the change by adjusting any time based operations
     * to the new time and rate.
     */
    struct TimeTravel {
        enum class Action : uint8_t {
            /// Adjust clock and move all chrono tasks with it
            RELATIVE,

            /// Adjust clock to target time and leave chrono tasks where they are
            ABSOLUTE,

            /// Adjust clock to as close to target as possible without skipping any chrono tasks
            NEAREST,
        };

        /// The target time to set the clock to
        clock::time_point target = clock::now();
        /// The rate at which time should pass
        double rtf = 1.0;
        /// The type of time travel to perform
        Action type = Action::RELATIVE;

        TimeTravel() = default;
        TimeTravel(const clock::time_point& target, double rtf = 1.0, Action type = Action::RELATIVE)
            : target(target), rtf(rtf), type(type) {}
    };

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_MESSAGE_TIME_TRAVEL_HPP
