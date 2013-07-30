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

namespace NUClear {
    
    // === Template Implementations ===
    // As with Reactor.h this section contains all template function implementations for ReactorMaster. 
    // Unfortunately it's neccecary to have these here and outside the NUClear namespace due to our circular dependency on Reactor.
    // What this means is: This code needs to be down here, it can't be moved into the class or namespace since that'll break the 
    // forward declaration resolution. See the similar comment in Reactor.h for more information.

    template <typename TReactor>
    void PowerPlant::ReactorMaster::install() {
        // The reactor constructor should handle subscribing to events
        std::unique_ptr<NUClear::Reactor> reactor(new TReactor(*m_parent));
        m_reactors.push_back(std::move(reactor));
    }

    template <typename TData>
    void PowerPlant::ReactorMaster::emit(TData* data) {
        
        // Get our current arguments (if we have any)
        auto task = m_parent->cachemaster.getCurrentTask(std::this_thread::get_id());
        
        // Cache them in our linked cache
        if(task) {
            m_parent->cachemaster.linkCache(data, task->m_args);
        }
        
        // Cache our data
        m_parent->cachemaster.cache<TData>(data);
        
        // Trigger all our reactions
        for(auto& reaction : CallbackCache<TData>::get()) {
            try {
                // Only run if our reaction is enabled
                if(reaction->isEnabled()) {
                    m_parent->threadmaster.submit(reaction->getTask(task));
                }
            }
            // If there is no data, then ignore the task
            catch (Internal::Magic::NoDataException) {}
        }
    }
}
