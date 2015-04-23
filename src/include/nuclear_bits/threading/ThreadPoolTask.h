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
#include "nuclear_bits/threading/TaskScheduler.h"

namespace NUClear {
    namespace threading {
        
        inline std::function<void ()> makeThreadPoolTask(PowerPlant& powerplant, TaskScheduler& scheduler) {
            return [&powerplant, &scheduler] {
                try {
                    // So long as we are executing
                    while(true) {
                        
                        // Get a task
                        std::unique_ptr<ReactionTask> task(scheduler.getTask());
                        
                        // Try to execute the task (catching any exceptions so it doesn't kill the pool thread)
                        try {
                            task->stats->started = clock::now();
                            (*task)();
                            task->stats->finished = clock::now();
                        }
                        // Catch everything
                        catch(...) {
                            task->stats->finished = clock::now();
                            task->stats->exception = std::current_exception();
                        }
                        
                        // Emit our ReactionStats
                        powerplant.emit<dsl::word::emit::Direct>(std::move(task->stats));
                    }
                }
                // If this is thrown, it means that we should finish execution
                catch (TaskScheduler::SchedulerShutdownException) {}
            };
        }
    }
}

#endif
