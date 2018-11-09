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

#ifndef NUCLEAR_THREADING_TASKSCHEDULER_HPP
#define NUCLEAR_THREADING_TASKSCHEDULER_HPP

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <typeindex>
#include <vector>

#include "Reaction.hpp"

namespace NUClear {
namespace threading {

    /**
     * @brief This class is responsible for scheduling tasks and distributing them amoungst threads.
     *
     * @details
     *  This task scheduler uses the options from each of the tasks to decide when to execute them in a thread. The
     * rules
     *  are applied to the tasks in the following order.
     *
     *  @em Priority
     *  @code Priority<P> @endcode
     *  When a priority is encountered, the task will be scheduled to execute based on this. If one of the three normal
     *  options are specified (HIGH, DEFAULT and LOW), then within the specified Sync group, it will run before,
     * normally
     *  or after other reactions.
     *  @attention Note that if Priority<REALTIME> is specified, the Sync type is ignored (Single is not).
     *
     *  @em Sync
     *  @code Sync<SyncGroup> @endcode
     *  When a Sync type is encounterd, the system uses this as a compile time mutex flag. It will not allow two
     * callbacks
     *  with the same Sync type to execute at the same time. It will effectivly ensure that all of the callbacks with
     *  this type run in sequence with eachother, rather then in parallell. It is also important to note again, that if
     *  the priority of a task is realtime, it will ignore Sync groups.
     *
     *  @em Single
     *  @code Single @endcode
     *  If single is encountered while processing the function, and a Task object for this Reaction is already running
     *  in a thread, or waiting in the Queue, then this task is ignored and dropped from the system.
     */
    class TaskScheduler {
    public:
        /**
         * @brief Constructs a new TaskScheduler instance, and builds the nullptr sync queue.
         */
        TaskScheduler();

        /**
         * @brief
         *  Shuts down the scheduler, all waiting threads are woken, and any attempt to get a task results in an
         *  exception
         */
        void shutdown();

        /**
         * @brief Submit a new task to be executed to the Scheduler.
         *
         * @details
         *  This method submits a new task to the scheduler. This task will then be sorted into the appropriate
         *  queue based on it's sync type and priority. It will then wait there until it is removed by a thread to
         *  be processed.
         *
         * @param task  the task to be executed
         */
        void submit(std::unique_ptr<ReactionTask>&& task);

        /**
         * @brief Get a task object to be executed by a thread.
         *
         * @details
         *  This method will get a task object to be executed from the queue. It will block until such a time as a
         *  task is available to be executed. For example, if a task with a paticular sync type was out, then this
         *  thread would block until that sync type was no longer out, and then it would take a task.
         *
         * @return the task which has been given to be executed
         */
        std::unique_ptr<ReactionTask> get_task();

    private:
        /// @brief if the scheduler is running or is shut down
        volatile bool running;
        /// @brief our queue which sorts tasks by priority
        std::priority_queue<std::unique_ptr<ReactionTask>> queue;
        /// @brief the mutex which our threads synchronize their access to this object
        std::mutex mutex;
        /// @brief the condition object that threads wait on if they can't get a task
        std::condition_variable condition;
    };

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_TASKSCHEDULER_HPP
