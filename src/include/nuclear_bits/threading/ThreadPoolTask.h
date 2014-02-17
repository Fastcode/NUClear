/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_THREADING_THREADPOOLTASK_H
#define NUCLEAR_THREADING_THREADPOOLTASK_H

#include "nuclear_bits/PowerPlant.h"
#include "nuclear_bits/threading/ThreadWorker.h"
#include "nuclear_bits/threading/TaskScheduler.h"

namespace NUClear {
    namespace threading {
        
        /**
         * @brief A task that executes thread pool reactions, they should make up most of the tasks in a system
         *
         * @author Trent Houliston
         */
        class ThreadPoolTask : public ThreadWorker::ServiceTask {
        public:
            
            /**
             * @brief Constructs a new ThreadPoolTask using a scheduler and powerplant
             *
             * @param powerplant the powerplant that this task is running under
             * @param scheduler the scheduler instance shared between the ThreadPool
             */
            ThreadPoolTask(PowerPlant& powerplant, TaskScheduler& scheduler);
            ~ThreadPoolTask();
            
            /// @brief runs the task, collecting and executing tasks from the scheduler
            void run();
            /// @breif kills the thread pool task, preventing future tasks from executing
            void kill();
        private:
            /// @brief the powerplant instance that this task is executing for
            PowerPlant& powerPlant;
            /// @breif the scheduler that is used to obtain tasks to execute
            TaskScheduler& scheduler;
        };
    }
}

#endif
