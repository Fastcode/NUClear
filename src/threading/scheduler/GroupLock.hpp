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
#ifndef NUCLEAR_THREADING_SCHEDULER_GROUP_LOCK_HPP
#define NUCLEAR_THREADING_SCHEDULER_GROUP_LOCK_HPP

#include <memory>

#include "Group.hpp"
#include "Lock.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        /**
         * Acts as a lock for a group.
         *
         * On calling lock(), the lock will attempt to obtain a token to execute tasks from the group.
         *
         * When the lock is destroyed, the token is released and any waiting pools are notified.
         */
        class GroupLock : public Lock {
        public:
            /**
             * Construct a new Group Lock object
             *
             * @param group the group to lock, may be nullptr if the lock is not required
             * @param pool  the pool to notify when the group is unlocked
             */
            GroupLock(const std::shared_ptr<Group>& group, const std::function<void()>& notifier = nullptr);

            /**
             * Destroy the Group Lock object
             *
             * This will release the token and notify any waiting pools.
             */
            ~GroupLock() override;

            GroupLock(const GroupLock&)            = delete;
            GroupLock& operator=(const GroupLock&) = delete;
            GroupLock(GroupLock&&)                 = delete;
            GroupLock& operator=(GroupLock&&)      = delete;

            /**
             * Attempts to obtain a token and lock the group.
             *
             * If group is nullptr, this will always return true.
             *
             * Once a token has been obtained, further calls to lock will return true but will not obtain a new token.
             *
             * @return true if the lock was able to obtain a token
             */
            virtual bool lock() override;

        private:
            /// The group to lock
            std::shared_ptr<Group> group;
            /// If the lock has obtained a token
            bool locked = false;

            /// The function to run after a failed lock to notify those interested
            std::function<void()> notifier;
            /// A handle which binds the notifier to the group if this is deleted the notifier won't be called
            std::shared_ptr<Group::WatcherHandle> watcher_handle;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_GROUP_LOCK_HPP
