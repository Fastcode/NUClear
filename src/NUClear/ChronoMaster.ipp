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

#include <algorithm>

namespace NUClear {
    
    template <int ticks, class period>
    void ReactorController::ChronoMaster::add() {
        // Check if we have not already loaded this type in
        if(m_loaded.find(typeid(NUClear::Internal::CommandTypes::Every<ticks, period>)) == std::end(m_loaded)) {
            
            // Flag this type as loaded
            m_loaded.insert(typeid(NUClear::Internal::CommandTypes::Every<ticks, period>));
            
            std::function<void (std::chrono::time_point<std::chrono::steady_clock>)> emit = [this](std::chrono::time_point<std::chrono::steady_clock> time){
                m_parent->reactormaster.emit(new NUClear::Internal::CommandTypes::Every<ticks, period>(time));
            };
            
            // Get our period in whatever time our clock measures
            std::chrono::steady_clock::duration step((period(ticks)));
            
            // See if we already have one with this period
            auto item = std::find_if(std::begin(m_steps), std::end(m_steps), [step](std::unique_ptr<Step>& find) {
                return find->step.count() == step.count();
            });
            
            // If we don't then create a new one with our initial data
            if(item == std::end(m_steps)) {
                std::unique_ptr<Step> s(new Step());
                s->step = step;
                s->callbacks.push_back(emit);
                m_steps.push_back(std::move(s));
            }
            
            // Otherwise just add the callback to the existing element
            else {
                (*item)->callbacks.push_back(emit);
            }
        }
    }
}
