/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#include "nuclear_bits/PowerPlant.h"
#include "nuclear_bits/threading/ThreadPoolTask.h"

namespace NUClear {

    const __thread threading::ReactionTask* PowerPlant::ThreadMaster::currentTask = nullptr;
    
    PowerPlant::ThreadMaster::ThreadMaster(PowerPlant& parent) :
    PowerPlant::BaseMaster(parent) {
    }
    
    void PowerPlant::ThreadMaster::start() {
        
        // Start our pool threads
        for(unsigned i = 0; i < parent.configuration.threadCount; ++i) {
            threads.push_back(std::make_unique<threading::ThreadWorker>(threading::ThreadPoolTask(parent, scheduler)));
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
    
    void PowerPlant::ThreadMaster::submit(std::unique_ptr<threading::ReactionTask>&& task) {
        scheduler.submit(std::forward<std::unique_ptr<threading::ReactionTask>>(task));
    }
}
