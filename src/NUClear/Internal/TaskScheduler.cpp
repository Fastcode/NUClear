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

#include "TaskScheduler.h"

namespace NUClear {
namespace Internal {
    
    TaskScheduler::TaskScheduler() : m_shutdown(false) {
    }
    
    TaskScheduler::~TaskScheduler() {
    }
    
    void TaskScheduler::shutdown() {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_shutdown = true;
        }
        m_condition.notify_all();
    }
    
    void TaskScheduler::submit(std::unique_ptr<Reaction::Task>&& task) {
        {
            // Obtain the lock
            std::unique_lock<std::mutex> lock(m_mutex);
            
            // We do not accept new tasks once we are shutdown or if this is a Single reaction that is already in the system
            if(!m_shutdown && (!task->m_parent->m_options.m_single || !task->m_parent->m_running)) {
                
                // We are now running
                task->m_parent->m_running = true;
                
                // If we are a sync type
                if(task->m_parent->m_options.m_syncQueue) {
                    
                    auto& queue = task->m_parent->m_options.m_syncQueue->m_queue;
                    auto& active = task->m_parent->m_options.m_syncQueue->m_active;
                    auto& mutex = task->m_parent->m_options.m_syncQueue->m_mutex;
                    
                    // Lock our sync types mutex
                    std::unique_lock<std::mutex> lock(mutex);
                    
                    // If a sync type is already executing then push it onto the sync queue
                    if (active) {
                        queue.push(std::move(task));
                    }
                    // Otherwise push it onto the main queue and set us to active
                    else {
                        active = true;
                        m_queue.push(std::move(task));
                    }
                }
                // Otherwise move it onto the main queue
                else {
                    m_queue.push(std::move(task));
                }
            }
        }
        
        // Notify a thread that it can proceed
        m_condition.notify_one();
    }
    
    std::unique_ptr<Reaction::Task> TaskScheduler::getTask() {
        
        //Obtain the lock
        std::unique_lock<std::mutex> lock(m_mutex);
        
        // How this works in practice is that it will not shut down a thread until all tasks are drained
        while (true) {
            // If there is nothing in the queue
            if (m_queue.empty()) {
                
                // And we are shutting down then terminate the requesting thread and tell all other threads to wake up
                if(m_shutdown) {
                    m_condition.notify_all();
                    throw TaskScheduler::SchedulerShutdownException();
                }
                else {
                    // Wait for something to happen!
                    m_condition.wait(lock);
                }
            }
            else {
                // Return the type
                // If you're wondering why all the rediculiousness, it's because priority queue is not as feature complete as it should be
                std::unique_ptr<Reaction::Task> task(std::move(const_cast<std::unique_ptr<Reaction::Task>&>(m_queue.top())));
                m_queue.pop();
                
                return std::move(task);
            }
        }
        
    }
}
}
