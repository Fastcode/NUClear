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
    
    namespace {
        bool compareQueues(const std::pair<const std::type_index&, TaskQueue&> a, const std::pair<const std::type_index&, TaskQueue&> b) {
            // If we are not active and not the default queue then we are smaller
            if(!a.second.m_active && a.first != typeid(nullptr)) {
                return false;
            }
            
            // If our queue is empty then return false
            else if(a.second.m_queue.empty()) {
                return false;
            }
            
            // If the other queue is empty then return true
            else if(b.second.m_queue.empty())
            {
                return true;
            }
            
            // If we have a higher priority then return true
            else if(a.second.m_queue.top()->m_options.m_priority > b.second.m_queue.top()->m_options.m_priority) {
                return true;
            }
            
            // If our event is older then return true
            else if(a.second.m_queue.top()->m_emitTime < b.second.m_queue.top()->m_emitTime) {
                return true;
            }
            
            //Otherwise assume that we are greater
            //(arbitrary pick should never happen unless two events are emitted at the exact same nanosecond?)
            else {
                return false;
            }
        }
    }
    
    TaskQueue::TaskQueue() : m_active(false) {
    }
    
    void TaskScheduler::submit(std::unique_ptr<ReactionTask>&& task) {
        
        (*task)();
        
        bool active = false;
        //if(task->m_options.m_single && !task->m_options.m_running) {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                
                // Put it in a queue according to it's sync type (typeid(nullptr) is treated as no sync type)
                active = m_queues[task->m_options.m_syncType].m_active;
                m_queues[task->m_options.m_syncType].m_queue.push(std::move(task));
            }
            if(active) {
                m_condition.notify_one();
            }
        //}
    }
    
    std::unique_ptr<ReactionTask> TaskScheduler::getTask() {
        
        //Obtain the lock
        std::unique_lock<std::mutex> lock(m_mutex);
        
        while(true) {
            // Get the queue we will be using (the active queue with the highest priority oldest element)
            auto queue = m_queues.begin();
            for(auto it = std::begin(m_queues); it != std::end(m_queues); ++it)
            {
                if(compareQueues(std::pair<const std::type_index&, TaskQueue&>(it->first, it->second), std::pair<const std::type_index&, TaskQueue&>(queue->first, queue->second))) {
                    queue = it;
                }
            }
            
            // If the queue is empty wait for notificiation
            if(queue->second.m_queue.empty()) {
                m_condition.wait(lock);
            }
            else {
                // Const cast is required here as for some reason the std::priority_queue only returns const references
                std::unique_ptr<ReactionTask> x(std::move(const_cast<std::unique_ptr<ReactionTask>&>(queue->second.m_queue.top())));
                queue->second.m_queue.pop();
                return std::move(x);
            }
        }
    }
}
}
