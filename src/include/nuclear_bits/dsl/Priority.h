/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
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

#ifndef NUCLEAR_DSL_PRIORITY_H
#define NUCLEAR_DSL_PRIORITY_H

namespace NUClear {
    
    /**
     * @ingroup Options
     * @brief Enum to specify which of the priorities to use for a task
     */
    enum EPriority {
        /// @brief Run the task immediantly, if there are no available threads in the thread pool, create a new thread to run this task
        REALTIME,
        /// @brief Run this task before other tasks once there is a free thread
        HIGH,
        /// @brief Run this task at the normal priority
        DEFAULT,
        /// @brief Run this task when there is a free pool thread, and no other tasks to use it
        LOW
    };
    
    namespace dsl {
        
        /**
         * @ingroup Options
         * @brief This option sets the priority of the task.
         *
         * @attention
         *  Due to the way the REALTIME option works in the system, it should be used sparingly as overuse
         *  will cause the system to spawn excessive threads and slowdown
         *
         * @details
         *  The priority option sets at which priority to allocate this task a thread in the thread pool. The options
         *  LOW DEFAULT and HIGH act this way, choosing tasks which have a higher priority before lower ones (unless
         *  their sync group is disabled). However Realtime works differently, ignoring the Sync group and ensuring
         *  that the task is executed immediantly. If there is no thread pool thread ready to execute this task. The
         *  system will spawn a new thread to ensure it happens.
         *
         * @tparam P the priority to run the task at (LOW DEFAULT HIGH and REALTIME)
         */
        template <enum EPriority P>
        struct Priority {
            Priority() = delete;
            ~Priority() = delete;
        };
    }
}

#endif
