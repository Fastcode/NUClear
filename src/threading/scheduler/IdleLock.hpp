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
#ifndef NUCLEAR_THREADING_SCHEDULER_IDLE_LOCK_HPP
#define NUCLEAR_THREADING_SCHEDULER_IDLE_LOCK_HPP

#include "Lock.hpp"
#include "Pool.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        /**
         * The idle lock class is designed to be used to lock such that it will return a lock for the thread which is
         * the last to check if it is idle (just before it sleeps).
         *
         * Once a thread has locked this lock as the last active thread, it will continue to hold the lock even if other
         * threads find themselves as the last active (due to race conditions). This is to ensure that only one thread
         * will execute the idle tasks.
         *
         * This is accomplished by setting the most significant bit of the active count to 1 when a thread is the last
         * to be active. Therefore other threads which will see this bit set and not consider themselves as the last.
         */
        class IdleLock : public Lock {
        public:
            using semaphore_t                 = unsigned int;
            static constexpr semaphore_t MASK = 1u << (sizeof(semaphore_t) * 8 - 1);

            /**
             * Construct a new Idle Lock object.
             *
             * Will attempt to acquire the idleness status for locked immediately upon construction as reattempting to
             * acquire the lock is meaningless as the answer should never change.
             *
             * @param active the atomic variable
             */
            IdleLock(std::atomic<semaphore_t>& active);
            ~IdleLock() override;

            IdleLock(const IdleLock&)            = delete;
            IdleLock& operator=(const IdleLock&) = delete;
            IdleLock(IdleLock&&)                 = delete;
            IdleLock& operator=(IdleLock&&)      = delete;

            /**
             * Return if this thread was the last active thread to check if it was idle.
             *
             * @return true if this thread was the last active thread when constructing this lock
             */
            bool lock() override;

        private:
            /// The current number of active threads
            std::atomic<semaphore_t>& active;

            /// If this thread was the last active thread and everything is now idle
            bool locked;
        };

        /**
         * A single lock which manages locking both the local and global idle locks.
         *
         * It also is able to specify which of the locks it was able to acquire.
         */
        class IdleLockPair : public Lock {
        public:
            IdleLockPair(std::atomic<unsigned int>& local_active, std::atomic<unsigned int>& global_active);
            ~IdleLockPair() override = default;

            IdleLockPair(const IdleLockPair&)            = delete;
            IdleLockPair& operator=(const IdleLockPair&) = delete;
            IdleLockPair(IdleLockPair&&)                 = delete;
            IdleLockPair& operator=(IdleLockPair&&)      = delete;

            bool lock() override;

            inline bool local_lock() {
                return local.lock();
            }
            inline bool global_lock() {
                return global.lock();
            }

        private:
            IdleLock local;
            IdleLock global;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_IDLE_LOCK_HPP
