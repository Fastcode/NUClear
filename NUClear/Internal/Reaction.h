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
