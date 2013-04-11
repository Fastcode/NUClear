#ifndef NUCLEAR_REACTION_H
#define NUCLEAR_REACTION_H
#include <functional>
#include <chrono>
#include <atomic>
#include <typeindex>
namespace NUClear {
namespace Internal {
    typedef std::uint64_t reactionId_t;
    
    struct ReactionOptions {
        ReactionOptions(std::type_index type) : m_syncType(type) {}
        std::type_index m_syncType;
        bool m_single;
    };
    
    class Reaction {
        public:
            Reaction(std::function<void ()> callback);
            ~Reaction();

            void operator()();
        private:
            static std::atomic<std::uint64_t> reactionIdSource;
            reactionId_t m_reactionId;
            std::uint64_t m_invocations;
            std::chrono::nanoseconds m_averageRuntime;
            std::function<void ()> m_callback;
    };
}
}
#endif
