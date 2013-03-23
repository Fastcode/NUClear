#ifndef NUCLEAR_REACTION_H
#define NUCLEAR_REACTION_H

#include <functional>
#include <typeindex>
#include <chrono>
#include <atomic>
#include <iostream>
#include <cstdint>

namespace NUClear {
    typedef std::int64_t reactionId_t;
    
    class Reaction {
    public:
        friend void swap(Reaction& first, Reaction& second);
        Reaction(std::function<void ()> callback, std::type_index type, reactionId_t parentId);
        Reaction(Reaction&& other);
        ~Reaction();

        Reaction(const Reaction&) = delete;

        Reaction& operator=(Reaction other);

        std::function<void ()> callback;
        std::type_index type;
        reactionId_t parentId;
        reactionId_t reactionId;
        std::chrono::time_point<std::chrono::steady_clock> emitTime;
        std::chrono::time_point<std::chrono::steady_clock> startTime;
        std::chrono::time_point<std::chrono::steady_clock> endTime;
    private:
        static std::atomic<int64_t> idSource;
    };
}

#endif
