/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#ifndef NUCLEAR_DSL_WORD_SYNC_HPP
#define NUCLEAR_DSL_WORD_SYNC_HPP

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This option sets the synchronisation for a group of tasks.
         *
         * @details
         *  @code on<Trigger<T, ...>, Sync<Group>>() @endcode
         *  When a group of tasks has been synchronised, only one task from the group will execute at a given time.
         *
         *  Should another task from this group be scheduled/requested (during execution of the current task), it will
         *  be sidelined into a priority queue.
         *
         *  Upon completion of the currently executing task, the queue will be polled to allow execution of the next
         *  task in this group.
         *
         *  Tasks in the synchronization queue are ordered based on their priority level, then their emission timestamp.
         *
         *  For best use, this word should be fused with at least one other binding DSL word.
         *
         * @par When should I use Sync
         *  Consider a reactor with a number of a reactions which modify its state.  It would be unwise to allow the
         *  reactions to run concurrently. To avoid race conditions, it is recommended that any reactions which modify
         *  the state be synced.
         *
         * @attention
         *  When using NUClear, developers should not make use of devices like a mutex. In the case of a mutex, threads
         *  will run and then block (leading to wasted resources on a number of inactive threads).  By using Sync,
         *  NUClear will have task and thread control so that system resources can be efficiently managed.
         *
         * @par Implements
         *  Pre-condition, Post-condition
         *
         * @tparam SyncGroup
         *  the type/group to synchronize on.  This needs to be a declared type within the system.  It is common to
         *  simply use the reactors name (i.e; if the reactor is only syncing with one group).  Should more than one
         *  group be required, the developer can declare structs within the system, to act as a group reference.
         *  Note that the developer is not limited to the use of a struct; any declared type will work.
         */
        template <typename SyncGroup>
        struct Sync {

            using task_ptr = std::unique_ptr<threading::ReactionTask>;

            /// @brief our queue which sorts tasks by priority
            static std::priority_queue<task_ptr> queue;
            /// @brief how many tasks are currently running
            static volatile bool running;
            /// @brief a mutex to ensure data consistency
            static std::mutex mutex;

            template <typename DSL>
            static inline std::unique_ptr<threading::ReactionTask> reschedule(
                std::unique_ptr<threading::ReactionTask>&& task) {

                // Lock our mutex
                std::lock_guard<std::mutex> lock(mutex);

                // If we are already running then queue, otherwise return and set running
                if (running) {
                    queue.push(std::move(task));
                    return std::unique_ptr<threading::ReactionTask>(nullptr);
                }
                else {
                    running = true;
                    return std::move(task);
                }
            }

            template <typename DSL>
            static void postcondition(threading::ReactionTask& task) {

                // Lock our mutex
                std::lock_guard<std::mutex> lock(mutex);

                // We are finished running
                running = false;

                // If we have another task, add it
                if (!queue.empty()) {
                    std::unique_ptr<threading::ReactionTask> next_task(
                        std::move(const_cast<std::unique_ptr<threading::ReactionTask>&>(queue.top())));
                    queue.pop();

                    // Resubmit this task to the reaction queue
                    task.parent.reactor.powerplant.submit(std::move(next_task));
                }
            }
        };

        template <typename SyncGroup>
        std::priority_queue<typename Sync<SyncGroup>::task_ptr> Sync<SyncGroup>::queue;

        template <typename SyncGroup>
        volatile bool Sync<SyncGroup>::running = false;

        template <typename SyncGroup>
        std::mutex Sync<SyncGroup>::mutex;

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_SYNC_HPP
