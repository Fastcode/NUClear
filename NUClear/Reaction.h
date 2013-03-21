#ifndef NUCLEAR_REACTION_H
#define NUCLEAR_REACTION_H

#include <functional>
#include <typeindex>
#include <ctime>
#include <atomic>

namespace NUClear {
    typedef std::int64_t reactionId_t;
    
    class Reaction {
    public:
        Reaction(std::type_index type);
        ~Reaction();
        std::function<void ()> callback;
        std::time_t emitTime;
        std::time_t startTime;
        std::time_t endTime;
        reactionId_t eventId;
        reactionId_t parentId;
        std::type_index type;
    private:
        static std::atomic<int64_t> idSource;
    };
}

#endif
