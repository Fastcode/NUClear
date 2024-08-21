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
#ifndef NUCLEAR_THREADING_SCHEDULER_COMBINED_LOCK
#define NUCLEAR_THREADING_SCHEDULER_COMBINED_LOCK

#include <memory>
#include <vector>

#include "Lock.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        /**
         * A lock which returns true if all of its sub locks return true.
         *
         * If there are no sub locks, this lock will always return true.
         */
        class CombinedLock : public Lock {
        public:
            CombinedLock() = default;
            explicit CombinedLock(std::unique_ptr<Lock>&& lock);
            ~CombinedLock() override = default;

            CombinedLock(const CombinedLock&)            = delete;
            CombinedLock& operator=(const CombinedLock&) = delete;
            CombinedLock(CombinedLock&&)                 = delete;
            CombinedLock& operator=(CombinedLock&&)      = delete;

            /**
             * Returns the aggregate status of all the locks.
             *
             * @return true if all of the component locks return true, false otherwise.
             */
            bool lock() override;

            /**
             * Adds a new lock to the combined lock.
             *
             * @param lock the lock to add
             */
            void add(std::unique_ptr<Lock>&& lock);

        private:
            std::vector<std::unique_ptr<Lock>> locks;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_COMBINED_LOCK
