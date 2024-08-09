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
#ifndef NUCLEAR_THREADING_SCHEDULER_COUNTING_LOCK_HPP
#define NUCLEAR_THREADING_SCHEDULER_COUNTING_LOCK_HPP

#include <atomic>

#include "Lock.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        /**
         * The counting lock will change an atomic variable and record the value upon construction.
         *
         * If it matches the target value, the lock() function will return true, otherwise it will return false.
         *
         * Upon destruction, the counter will be decremented by the step value to undo the transformation.
         */
        class CountingLock : public Lock {
        public:
            /**
             * Construct a new Counting Lock object.
             *
             * It will increment the counter by the step value and store if it has hit the target value as locked.
             *
             * @param counter the atomic variable
             * @param step    the amount to increment the counter by
             * @param target  the target value to hit
             */
            CountingLock(std::atomic<int>& counter, const int& step = -1, const int& target = 0);

            ~CountingLock() override;

            CountingLock(const CountingLock&)            = delete;
            CountingLock& operator=(const CountingLock&) = delete;
            CountingLock(CountingLock&&)                 = delete;
            CountingLock& operator=(CountingLock&&)      = delete;

            /**
             * Return this lock hit the target upon construction.
             *
             * @return true if this thread was the last active thread when constructing this lock
             */
            bool lock() override;

        private:
            /// The atomic variable to mutate
            std::atomic<int>& counter;
            /// The amount to increment the counter by
            const int step;  // NOLINT()

            /// If this lock hit the target when it was constructed
            bool locked;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_COUNTING_LOCK_HPP
