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

#include "NUClear/ReactorController.h"

namespace NUClear {
    
    ReactorController::ChronoMaster::ChronoMaster(ReactorController* parent) :
    ReactorController::BaseMaster(parent),
    m_lock(m_execute) {
        
        // Build a task
        Internal::ThreadWorker::InternalTask task(std::bind(&ChronoMaster::run, this),
                                                  std::bind(&ChronoMaster::kill, this));
        
        m_parent->threadmaster.internalTask(task);
    }

    ReactorController::ChronoMaster::~ChronoMaster() {
    }

    void ReactorController::ChronoMaster::run() {
        if(!m_steps.empty()) {
            // Initialize all of the m_steps with our start time
            std::chrono::time_point<std::chrono::steady_clock> start(std::chrono::steady_clock::now());
            for(auto& step : m_steps) {
                step->next = start;
            }
            
            do {
                // Get the current time
                std::chrono::time_point<std::chrono::steady_clock> now(std::chrono::steady_clock::now());
                
                // Check if any intervals are before now and if so execute their callbacks and add their step.
                for(auto& step : m_steps) {
                    if((step->next - now).count() <= 0) {
                        for(auto& callback : step->callbacks) {
                            callback(now);
                        }
                        step->next += step->step;
                    }
                    // Since we are sorted, we can ignore any after this time
                    else {
                        break;
                    }
                }
                
                // Sort the list so the next soonest interval is on top
                std::sort(std::begin(m_steps), std::end(m_steps), [](const std::unique_ptr<Step>& a, const std::unique_ptr<Step>& b) {
                    return a->next < b->next;
                });
            }
            // Sleep until it's time to emit this event, or the lock is released and we should finish
            while(!m_execute.try_lock_until(m_steps.front()->next));
        }
    }
    
    void ReactorController::ChronoMaster::kill() {
        //If we unlock the lock, then the lock will return true, ending the chronomasters run method
        m_lock.unlock();
    }
}
