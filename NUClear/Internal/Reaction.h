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
#ifndef NUCLEAR_REACTION_H
#define NUCLEAR_REACTION_H
#include <functional>
#include <chrono>
#include <atomic>
#include <typeindex>
#include "Options.h"
namespace NUClear {
namespace Internal {
    typedef std::uint64_t reactionId_t;
    class Reaction;
    
    struct ReactionOptions {
        ReactionOptions() : m_syncType(typeid(nullptr)), m_single(false), m_priority(DEFAULT) {}
        std::type_index m_syncType;
        bool m_single;
        EPriority m_priority;
    };
    
    class ReactionTask {
        public:
            ReactionTask(Reaction& parent, std::function<void ()> task);
            void operator()();
            ReactionOptions& m_options;
        private:
            std::function<void ()> m_callback;
            Reaction& m_parent;
            std::chrono::nanoseconds m_runtime;
    };
    
    class Reaction {
        public:
            Reaction(std::function<std::function<void ()> ()> callback, ReactionOptions options);
            ~Reaction();
            ReactionOptions m_options;
            std::unique_ptr<ReactionTask> getTask();
        private:
            static std::atomic<std::uint64_t> reactionIdSource;
            reactionId_t m_reactionId;
            std::chrono::nanoseconds m_averageRuntime;
            std::function<std::function<void ()> ()> m_callback;
    };
}
}
#endif
