/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_SLEEPER_HPP
#define NUCLEAR_UTIL_SLEEPER_HPP

#include <atomic>
#include <chrono>
#include <memory>

namespace NUClear {
namespace util {

    struct SleeperState;

    /**
     * A class that provides platform independent precise sleep functionality.
     *
     * The Sleeper class allows for sleeping for a specified duration of time.
     * It will use the most accurate method available on the platform to sleep for the specified duration.
     * It will then spin the CPU for the remaining time to ensure that the sleep is as accurate as possible.
     */
    class Sleeper {
    public:
        Sleeper();
        ~Sleeper();

        Sleeper(const Sleeper& other)            = delete;
        Sleeper& operator=(const Sleeper& other) = delete;
        Sleeper(Sleeper&& other)                 = delete;
        Sleeper& operator=(Sleeper&& other)      = delete;

        /**
         * Sleep for the specified duration.
         *
         * @param duration The duration to sleep for.
         */
        void sleep_for(const std::chrono::nanoseconds& duration) {
            sleep_until(std::chrono::steady_clock::now() + duration);
        }

        /**
         * Sleep until the specified time point.
         *
         * @param target The time point to sleep until.
         */
        void sleep_until(const std::chrono::steady_clock::time_point& target);

        /**
         * Wake up the sleeper early, or if it was already awake, make it not sleep next time it tries.
         */
        void wake();

    private:
        std::unique_ptr<SleeperState> state;
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_SLEEPER_HPP
