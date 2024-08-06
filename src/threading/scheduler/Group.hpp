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
#ifndef NUCLEAR_THREADING_SCHEDULER_GROUP_HPP
#define NUCLEAR_THREADING_SCHEDULER_GROUP_HPP

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "../../util/GroupDescriptor.hpp"
#include "Lock.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        struct Pool;

        /**
         * A group is a collection of tasks which are mutually exclusive to each other.
         *
         * They are identified by having a common group id along with a maximum concurrency.
         * This class holds the structures that manage the group.
         *
         * This class is used along with the GroupLock class to manage the group locking.
         */
        struct Group {
            /**
             * Represents a watcher that is waiting for a token to be available.
             *
             * Called will be set to true when the function is called and the watcher will be removed from the queue.
             * This handle is stored as a weak pointer inside the group so that if the watcher is destroyed it will be
             * able to be ignored.
             */
            struct WatcherHandle {
                /// Construct a new WatcherHandle object
                explicit WatcherHandle(const std::function<void()>& fn) : fn(fn) {}
                /// The function to call when a token is available
                std::function<void()> fn;
                /// If the function has been called already
                bool called = false;
            };

            /**
             * Construct a new Group object
             *
             * @param descriptor The descriptor for this group
             */
            explicit Group(const util::GroupDescriptor& descriptor);

            /**
             * This function adds a watcher to the group.
             *
             * This is typically a thread pool that has a task in its queue that that depends on this group.
             * If the group was blocked when it tried to run the task then the thread pool may go to sleep.
             * The function passed here should be called when a token is available so that it can be woken up.
             *
             * The function will be removed after being called once, so if the pool was still unable to run the task it
             * should re-add itself.
             *
             * @param fn The function to execute when a token becomes available
             *
             * @return A shared pointer that will remove the watcher when it is destroyed
             */
            std::shared_ptr<WatcherHandle> add_watcher(const std::function<void()>& fn);

            /**
             * Notify all watchers that a token is available to be claimed.
             */
            void notify();

            /// The descriptor for this group
            const util::GroupDescriptor descriptor;

            /// The number of tokens available locks for this group
            std::atomic<int> tokens{int(descriptor.thread_count)};

            /// The mutex which protects the queue
            std::mutex mutex;
            /// The queue of tasks for this specific thread pool and if they are group blocked
            std::vector<std::weak_ptr<WatcherHandle>> watchers;
        };

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_SCHEDULER_GROUP_HPP
