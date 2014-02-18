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

#include "nuclear_bits/threading/ThreadPoolTask.h"

namespace NUClear {
    namespace threading {
        
        ThreadPoolTask::ThreadPoolTask(PowerPlant& powerplant, TaskScheduler& scheduler) :
        ThreadWorker::ServiceTask(std::bind(&ThreadPoolTask::run, this), std::bind(&ThreadPoolTask::kill, this)),
        powerplant(powerplant),
        scheduler(scheduler) {
        }
        
        ThreadPoolTask::~ThreadPoolTask() {
        }
        
        void ThreadPoolTask::run() {
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
                    
                    // We have stopped running
                    task->parent->running = false;
                    
                    // If we are a sync type
                    if(task->parent->options.syncQueue) {
                        
                        auto& queue = task->parent->options.syncQueue->queue;
                        auto& active = task->parent->options.syncQueue->active;
                        auto& syncMutex = task->parent->options.syncQueue->mutex;
                        
                        active = false;
                        
                        // If there is something in our sync queue move it to the main queue
                        if (!queue.empty()) {
                            
                            std::unique_ptr<ReactionTask> syncTask;
                            {
                                // Lock our sync types mutex
                                std::lock_guard<std::mutex> lock(syncMutex);
                                syncTask = std::move(const_cast<std::unique_ptr<ReactionTask>&>(queue.top()));
                                queue.pop();
                            }
                            
                            scheduler.submit(std::move(syncTask));
                        }
                    }
                    
                    // Emit our ReactionStats
                    powerplant.emit<dsl::Scope::DIRECT>(std::move(task->stats));
                }
            }
            // If this is thrown, it means that we should finish execution
            catch (TaskScheduler::SchedulerShutdownException) {}
        }
        
        void ThreadPoolTask::kill() {
            // We don't do anything on being killed, the scheduler handles our demise
        }
        
    }
}
