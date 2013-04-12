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
#include "ReactorController.h"
namespace NUClear {
    ReactorController::ChronoMaster::ChronoMaster(ReactorController* parent) :
        ReactorController::BaseMaster(parent),
        m_execute(true) {}

    ReactorController::ChronoMaster::~ChronoMaster() {
        m_execute = false;
    }

    void ReactorController::ChronoMaster::run() {
        // Initialize all of the m_steps with our start time
        std::chrono::nanoseconds start(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()));
        for(auto it = std::begin(m_steps); it != std::end(m_steps); ++it) {
            (*it)->next = start;
        }
        
        // Loop until it is time for us to finish
        while(m_execute) {
            // Get the current time
            std::chrono::nanoseconds now(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()));
            
            // Check if any intervals are before now and if so execute their callbacks and add their step.
            for(auto it = std::begin(m_steps); it != std::end(m_steps); ++it) {
                if(((*it)->next - now).count() <= 0) {
                    for(auto callback = std::begin((*it)->callbacks); callback != std::end((*it)->callbacks); ++callback) {
                        (*callback)();
                    }
                    (*it)->next += (*it)->step;
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
            
            // Sleep until it's time to emit this event
            std::this_thread::sleep_for(m_steps.front()->next - now);
        }
    }
}
