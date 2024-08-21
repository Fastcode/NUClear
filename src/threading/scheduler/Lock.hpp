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
#ifndef NUCLEAR_THREADING_SCHEDULER_LOCK_HPP
#define NUCLEAR_THREADING_SCHEDULER_LOCK_HPP

namespace NUClear {
namespace threading {
    namespace scheduler {

        /**
         * Represents an RAII lock that can be attached to a task.
         *
         * This lock is able to determine if a task should be executed based on the current state of the system.
         * Different implementations of lock may be used to ensure different properties of the system are maintained.
         * Additionally since this lock is RAII, it will ensure that the lock is held until the task has finished
         * execution, not just until the task is removed from the queue.
         */
        class Lock {
        public:
            Lock()          = default;
            virtual ~Lock() = default;

            Lock(const Lock&)            = delete;
            Lock& operator=(const Lock&) = delete;
            Lock(Lock&&)                 = delete;
            Lock& operator=(Lock&&)      = delete;

            /**
             * Returns the current status of the lock, potentially attempting to acquire it.
             * This function must be non blocking.
             *
             * @return True if the lock was successfully acquired, false otherwise.
             */
            virtual bool lock();
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_LOCK_HPP
