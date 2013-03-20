#ifndef NUCLEAR_REACTION_H
#define NUCLEAR_REACTION_H

#include <functional>
#include <typeindex>
#include <ctime>

namespace NUClear {
    typedef std::int64_t taskId_t;
    
    class Reaction {
    public:
        Reaction(std::type_index type);
        ~Reaction();
        std::function<void ()> callback;
        std::time_t emitTime;
        std::time_t startTime;
        std::time_t endTime;
        taskId_t eventId;
        taskId_t parentId;
        std::type_index type;
    };
}

#endif