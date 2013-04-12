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
