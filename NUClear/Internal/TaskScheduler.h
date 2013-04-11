#ifndef NUCLEAR_TASKSCHEDULER_H
#define NUCLEAR_TASKSCHEDULER_H

#include <map>
#include <vector>
#include <atomic>
#include <queue>
#include <algorithm>
#include <typeindex>
#include <condition_variable>
#include <mutex>
#include "Reaction.h"

namespace NUClear {
namespace Internal {
    
    /*class TaskQueue {
        public:
            TaskQueue();
            std::priority_queue<Reaction> m_queue;
            std::type_index m_syncType;
            std::atomic<bool> active;
        //TODO overload the < operator so it can be heaped
    };*/

    class TaskScheduler {
        public:
            void submit(Reaction& r);
            //Reaction& getTask();
        private:
            //std::map<std::type_index, TaskQueue> m_queues;
            //std::mutex m_mutex;
            //std::condition_variable m_condition;
        
    };

}
}

/**
 * Options that the scheduler should be able to take
 *
 * Sync<typename type>      this will allow only one reaction from a paticular sync group
 *                          to be given to the pool at any one time
 *
 * Priority<int priority>   this will schedule reactions according to priority, a priority of 0
 *                          means realtime, it must be run now whatever the cost (perhaps declare
 *                          some constants for levels or use an enum)
 *
 * Single                   this will only allow a single reaction of this type to exist at any one time
 *                          if a reaction with this is triggered while another one is either executing, or enqueued this
 *                          trigger will be ignored
 * 
 * Filter<typename type>    This option should not be exposed directly to the user, but it should be used for filter
 *                          reactions. Each filter reaction should be run on an event in sequence (non parallel) and when
 *                          all of the filter reactions are finished then start running the real reactions on the event
 */

#endif
