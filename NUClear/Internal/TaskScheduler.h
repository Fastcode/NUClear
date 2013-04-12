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
    
    class TaskQueue {
        public:
            TaskQueue();
            std::priority_queue<std::unique_ptr<ReactionTask>> m_queue;
            std::atomic<bool> m_active;
            bool operator<(TaskQueue& other);
        //TODO overload the < operator so it can be heaped
    };

    class TaskScheduler {
        public:
            void submit(std::unique_ptr<ReactionTask>&& task);
            std::unique_ptr<ReactionTask> getTask();
        private:
            std::map<std::type_index, TaskQueue> m_queues;
            std::mutex m_mutex;
            std::condition_variable m_condition;
        
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
