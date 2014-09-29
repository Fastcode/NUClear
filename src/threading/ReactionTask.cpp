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

#include "nuclear_bits/threading/ReactionTask.h"
#include "nuclear_bits/threading/Reaction.h"

namespace NUClear {
    namespace threading {
        
        // Initialize our id source
        std::atomic<uint64_t> ReactionTask::taskIdSource(0);
        
        ReactionTask::ReactionTask(Reaction* parent, const ReactionTask* cause, std::function<void (ReactionTask&)> callback) :
        callback(callback),
        parent(parent),
        taskId(++taskIdSource),
        stats(new ReactionStatistics {
              parent->identifier
            , parent->reactionId
            , taskId
            , cause ? cause->parent->reactionId : 0
            , cause ? cause->taskId : 0
            , clock::now()
            , clock::time_point(std::chrono::seconds(0))
            , clock::time_point(std::chrono::seconds(0))
            , nullptr
        }) {}
        
        void ReactionTask::operator()() {
            // Call our callback
            callback(*this);
        }
    }
}
