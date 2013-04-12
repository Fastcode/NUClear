/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, 
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software 
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR 
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */ 
#include "TaskScheduler.h"
namespace NUClear {
namespace Internal {
    
    TaskQueue::TaskQueue() : m_syncType(typeid(nullptr)), m_active(false) {
        
    }
    
    void TaskScheduler::submit(std::unique_ptr<ReactionTask>&& task) {
        
        // Check if it's a single and it is running ( TODO: Running check )
        if(task->m_options.m_single) {
            
            std::unique_lock<std::mutex> lock(m_mutex);
            
            // Put it in a queue according to it's sync type (typeid(nullptr) is treated as no sync type)
            m_queues[task->m_options.m_syncType].m_queue.push(std::move(task));
            
            // Check if this queue is active and if so then notify a thread
        }
    }
    
    std::unique_ptr<ReactionTask> TaskScheduler::getTask() {
        
        // Get the lock
        // while we don't have a task
        // std::max_element(begin, end, comparator) this puts the queue to use at the top
        // perform make_heap on the queues
        // conditinally get from the top queue (or wait if empty)
        // Return the task
        // endwhile
        
    }
    
}}
