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

#include "NUClear/Internal/ThreadPoolTask.h"

namespace NUClear {
namespace Internal {
    
    ThreadPoolTask::ThreadPoolTask(TaskScheduler& scheduler) :
    ThreadWorker::ServiceTask(std::bind(&ThreadPoolTask::run, this), std::bind(&ThreadPoolTask::kill, this)),
    m_scheduler(scheduler) {
    }
    
    ThreadPoolTask::~ThreadPoolTask() {
    }
    
    void ThreadPoolTask::run() {
        try {
            // So long as we are executing
            while(true) {
                
                // Get a task
                std::unique_ptr<Reaction::Task> task(m_scheduler.getTask());
                std::shared_ptr<NUClearTaskEvent> stats = task->m_stats;
                
                // Try to execute the task (catching any exceptions so it doesn't kill the pool thread)
                try {
                    stats->started = clock::now();
                    (*task)();
                    stats->finished = clock::now();
                }
                // Catch everything
                catch(...) {
                    stats->finished = clock::now();
                    stats->exception = std::current_exception();
                }
                
                // We have stopped running
                task->m_parent->m_running = false;
                
                // If we are a sync type
                if(task->m_parent->m_options.m_syncQueue) {
                    
                    auto& queue = task->m_parent->m_options.m_syncQueue->m_queue;
                    auto& active = task->m_parent->m_options.m_syncQueue->m_active;
                    auto& mutex = task->m_parent->m_options.m_syncQueue->m_mutex;
                    
                    // Lock our sync types mutex
                    std::unique_lock<std::mutex> lock(mutex);
                    
                    // If there is something in our sync queue move it to the main queue
                    if (!queue.empty()) {
                        std::unique_ptr<Reaction::Task> syncTask(std::move(const_cast<std::unique_ptr<Reaction::Task>&>(queue.top())));
                        queue.pop();
                        
                        m_scheduler.submit(std::move(syncTask));
                    }
                    // Otherwise set active to false
                    else {
                        active = false;
                    }
                }
                
                //TODO pass off the completed task to another class for processing of details
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
