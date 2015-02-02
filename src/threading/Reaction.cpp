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

#include "nuclear_bits/threading/Reaction.h"

namespace NUClear {
    namespace threading {
        
        // Initialize our reaction source
        std::atomic<uint64_t> Reaction::reactionIdSource(0);
        
        Reaction::Reaction(std::vector<std::string> identifier
                           , std::function<std::function<void ()> (ReactionTask&)> generator
                           , bool (*precondition)(Reaction&)
                           , int (*priority)(Reaction&)
                           , void (*postcondition)(ReactionTask&)
                           , std::function<void (Reaction&)>&& unbinder)
          : identifier(identifier)
          , reactionId(++reactionIdSource)
          , activeTasks(0)
          , enabled(true)
          , precondition(precondition)
          , priority(priority)
          , postcondition(postcondition)
          , generator(generator)
          , unbinder(unbinder) {
        }
        
        void Reaction::unbind() {
            // Unbind
            unbinder(*this);
        }
        
        std::unique_ptr<ReactionTask> Reaction::getTask(const ReactionTask* cause) {
            
            // Lock our mutex for our precondition
            if(precondition(*this)) {
                return std::unique_ptr<ReactionTask>(new ReactionTask(*this, cause, priority(*this), generator));
            }
            else {
                throw std::runtime_error("Task is unable to be created as the precondition fails");
            }
        }
        
        bool Reaction::isEnabled() {
            return enabled;
        }
    }
}
