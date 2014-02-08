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

#include "nuclear_bits/threading/TaskScheduler.h"

namespace NUClear {
    namespace threading {
        
        TaskScheduler::TaskScheduler() : shutdown_(false) {
        }
        
        TaskScheduler::~TaskScheduler() {
        }
        
        void TaskScheduler::shutdown() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                shutdown_ = true;
            }
            condition.notify_all();
        }
        
        void TaskScheduler::submit(std::unique_ptr<ReactionTask>&& task) {
            {
                // We do not accept new tasks once we are shutdown or if this is a Single reaction that is already in the system
                if(!shutdown_ && (!task->parent->options.single || !task->parent->running)) {
                    
                    // We are now running
                    task->parent->running = true;
                    
                    // If we are a sync type
                    if(task->parent->options.syncQueue) {
                        
                        auto& syncQueue = task->parent->options.syncQueue->queue;
                        auto& syncMutex = task->parent->options.syncQueue->mutex;
                        auto& active = task->parent->options.syncQueue->active;
                        
                        // Lock access to our syncQueue
                        std::lock_guard<std::mutex> lock(syncMutex);
                        
                        // If a sync type is already executing then push it onto the sync queue
                        if (active) {
                            syncQueue.push(std::forward<std::unique_ptr<ReactionTask>>(task));
                        }
                        // Otherwise push it onto the main queue and set us to active
                        else {
                            active = true;
                            std::lock_guard<std::mutex> lock(mutex);
                            queue.push(std::forward<std::unique_ptr<ReactionTask>>(task));
                        }
                    }
                    // Otherwise move it onto the main queue
                    else {
                        std::lock_guard<std::mutex> lock(mutex);
                        queue.push(std::forward<std::unique_ptr<ReactionTask>>(task));
                    }
                }
            }
            
            // Notify a thread that it can proceed
            condition.notify_one();
        }
        
        std::unique_ptr<ReactionTask> TaskScheduler::getTask() {
            
            //Obtain the lock
            std::unique_lock<std::mutex> lock(mutex);
            
            // How this works in practice is that it will not shut down a thread until all tasks are drained
            while (true) {
                // If there is nothing in the queue
                if (queue.empty()) {
                    
                    // And we are shutting down then terminate the requesting thread and tell all other threads to wake up
                    if(shutdown_) {
                        condition.notify_all();
                        throw TaskScheduler::SchedulerShutdownException();
                    }
                    else {
                        // Wait for something to happen!
                        condition.wait(lock);
                    }
                }
                else {
                    // Return the type
                    // If you're wondering why all the rediculiousness, it's because priority queue is not as feature complete as it should be
                    std::unique_ptr<ReactionTask> task(std::move(const_cast<std::unique_ptr<ReactionTask>&>(queue.top())));
                    queue.pop();
                    
                    return std::move(task);
                }
            }
            
        }
    }
}
