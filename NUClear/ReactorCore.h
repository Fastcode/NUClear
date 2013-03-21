#ifndef NUCLEAR_REACTORCORE_H
#define NUCLEAR_REACTORCORE_H

#include <functional>
#include <map>
#include <queue>
#include <typeindex>
#include <memory>
#include "Reaction.h"
#include "ReactionQueue.h"
#include "ExecutionCore.h"

namespace NUClear {
    class ReactorCore {
        public:
            ReactorCore();
            ~ReactorCore();
            void submit(Reaction task);
            reactionId_t getCurrentReactionId(std::thread::id threadId);
        private:
            std::map<std::thread::id, std::unique_ptr<ExecutionCore>> cores;
            ReactionQueue queue;
    };
}

#endif
