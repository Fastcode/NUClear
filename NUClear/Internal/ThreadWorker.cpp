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

#include "ThreadWorker.h"

namespace NUClear {
namespace Internal {
    
    ThreadWorker::ThreadWorker(TaskScheduler& scheduler) :
    m_scheduler(scheduler),
    m_execute(true),
    m_currentReaction(nullptr),
    m_thread(std::bind(&ThreadWorker::core, this)) {
    }
    
    ThreadWorker::~ThreadWorker() {
        // This should never happen, but if we are destructed, stop running
        m_execute = false;
    }
    
    void ThreadWorker::kill() {
        // Set our running status to false
        m_execute = false;
    }
    
    void ThreadWorker::join() {
        
        // only join the thread if it's joinable (or errors!)
        if(m_thread.joinable()) {
            m_thread.join();
        }
    }
    
    std::thread::id ThreadWorker::getThreadId() {
        // get the thread id from our internal thread
        return m_thread.get_id();
    }
    
    ReactionTask& ThreadWorker::getCurrentReaction() {
        // we can safely dereference here, because this thread should only be called from our own thread while we are
        // executing a reaction. Therefore if this method was called by us, then it is not null
        return *m_currentReaction;
    }
    
    void ThreadWorker::core() {
        // So long as we are executing
        while(m_execute) {
            // Get a task
            std::unique_ptr<ReactionTask> task(m_scheduler.getTask());
            
            // Try to execute the task (catching any exceptions so it doesn't kill the pool thread)
            try {
                (*task)();
            }
            // Catch everything
            catch(...) {}
        }
    }
}
}
