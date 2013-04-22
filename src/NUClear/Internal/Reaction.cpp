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

#include "Reaction.h"

namespace NUClear {
namespace Internal {
    
    std::atomic<uint64_t> Reaction::reactionIdSource(0);
    
    Reaction::Reaction(std::function<std::pair<std::function<void ()>, std::vector<std::pair<std::type_index, std::shared_ptr<void>>>> ()> callback, Reaction::Options options) :
    m_options(options),
    m_reactionId(++reactionIdSource),
    m_running(false),
    m_callback(callback) {
    }
    
    std::unique_ptr<Reaction::Task> Reaction::getTask() {
        // Build a new data bound task using our callback generator
        return std::unique_ptr<Reaction::Task>(new Reaction::Task(this, m_callback()));
    }
    
    Reaction::Task::Task(Reaction* parent, std::pair<std::function<void ()>, std::vector<std::pair<std::type_index, std::shared_ptr<void>>>> args) :
    m_parent(parent),
    m_callback(args.first),
    m_args(args.second),
    m_emitTime(std::chrono::steady_clock::now()) {
    }
    
    void Reaction::Task::operator()() {
        
        // Start timing and execute our function
        m_startTime = std::chrono::steady_clock::now();
        m_callback();
        
        // Finish timing and calculate our runtime
        const auto& end = std::chrono::steady_clock::now();
        m_runtime = end.time_since_epoch() - m_startTime.time_since_epoch();
    }
}
}
