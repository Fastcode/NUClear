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

#include "nuclear_bits/threading/Reaction.h"

namespace NUClear {
namespace Internal {
    
    std::atomic<uint64_t> Reaction::reactionIdSource(0);
    std::atomic<uint64_t> Reaction::Task::taskIdSource(0);
    
    
    Reaction::Reaction(std::string name, std::function<std::function<void (Reaction::Task&)> ()> callback, Reaction::Options options) :
    name(name),
    options(options),
    reactionId(++reactionIdSource),
    running(false),
    enabled(true),
    callback(callback) {
    }
    
    std::unique_ptr<Reaction::Task> Reaction::getTask(const Task* cause) {
        // Build a new data bound task using our callback generator
        return std::unique_ptr<Reaction::Task>(new Reaction::Task(this, cause, callback()));
    }
    
    bool Reaction::isEnabled() {
        return enabled;
    }
    
    Reaction::OnHandle::OnHandle(Reaction* context) : context(context) {
    };
    
    bool Reaction::OnHandle::isEnabled() {
        return context->enabled;
    }
    
    void Reaction::OnHandle::enable() {
        context->enabled = true;
    }
    
    void Reaction::OnHandle::disable() {
        context->enabled = false;
    }
    
    Reaction::Task::Task(Reaction* parent, const Task* cause, std::function<void (Task&)> callback) :
    callback(callback),
    parent(parent),
    taskId(++taskIdSource),
    stats(new NUClearTaskEvent{parent->name, parent->reactionId, taskId, cause ? cause->parent->reactionId : -1, cause ? cause->taskId : -1, clock::now()}) {
    }
    
    void Reaction::Task::operator()() {
        // Call our callback
        callback(*this);
    }
}
}
