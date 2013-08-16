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

#include "NUClear/PowerPlant.h"
#include "NUClear/Internal/ThreadPoolTask.h"

namespace NUClear {
    
    PowerPlant::ThreadMaster::ThreadMaster(PowerPlant* parent) :
    PowerPlant::BaseMaster(parent) {
    }


    void PowerPlant::ThreadMaster::setCurrentTask(std::thread::id threadId, const Internal::Reaction::Task* task) {
        // TODO replace this with thread_local if possible (c++11 keyword thread_local)
        currentTask[threadId] = task;
    }

    const Internal::Reaction::Task* PowerPlant::ThreadMaster::getCurrentTask(std::thread::id threadId) {
        auto task = currentTask.find(threadId);

        if(task != std::end(currentTask)) {
            return task->second;
        }
        else {
            return nullptr;
        }
    }

    void PowerPlant::ThreadMaster::start() {
        // Start our internal service threads
        for(auto& task : serviceTasks) {
            // Start a thread worker with our task
            std::unique_ptr<Internal::ThreadWorker> thread = std::unique_ptr<Internal::ThreadWorker>(new Internal::ThreadWorker(task));
            threads.push_back(std::move(thread));
        }
        
        // Start our pool threads
        for(unsigned i = 0; i < parent->configuration.threadCount; ++i) {
            std::unique_ptr<Internal::ThreadWorker> thread = std::unique_ptr<Internal::ThreadWorker>(new Internal::ThreadWorker(Internal::ThreadPoolTask(scheduler)));
            threads.push_back(std::move(thread));
        }
        
        // Now wait for all the threads to finish executing
        for(auto& thread : threads) {
            thread->join();
        }
    }
    
    void PowerPlant::ThreadMaster::shutdown() {
        // Kill everything
        for(auto& thread : threads) {
            thread->kill();
        }
        // Kill the task scheduler
        scheduler.shutdown();
    }
    
    void PowerPlant::ThreadMaster::serviceTask(Internal::ThreadWorker::ServiceTask task) {
        serviceTasks.push_back(task);
    }
    
    void PowerPlant::ThreadMaster::submit(std::unique_ptr<Internal::Reaction::Task>&& task) {
        scheduler.submit(std::move(task));
    }
}
