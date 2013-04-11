#include "TaskScheduler.h"
namespace NUClear {
namespace Internal {
    
    /*TaskQueue::TaskQueue() { //: m_syncType(typeid(nullptr)) {
        
    }
    
    void TaskScheduler::submit(Reaction& r) {
        
        // Check if it's a single and it is running ( TODO: Running check )
        /*if(r.m_options.m_single) {
            
            std::unique_lock<std::mutex> lock(m_mutex);
            
            // Put it in a queue according to it's sync type (typeid(nullptr) is treated as no sync type)
            m_queues[r.m_options.m_syncType].m_queue.push(std::move(r));
            
            // Check if this queue is active and if so then notify a thread
        }
    }*/
    
    /*Reaction& TaskScheduler::getTask() {
        
        // Get the lock
        // while we don't have a task
        // std::make_heap(begin, end, comparator) this puts the queue to use at the top
        // perform make_heap on the queues
        // conditinally get from the top queue (or wait if empty)
        // Return the task
        // endwhile
        
    }*/
    
}}
