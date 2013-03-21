#ifndef NUCLEAR_REACTORCORE_H
#define NUCLEAR_REACTORCORE_H

#include <functional>
#include <map>
#include <queue>
#include <typeindex>
#include "Reaction.h"
#include "ReactorTaskQueue.h"
#include "ExecutionCore.h"

namespace NUClear {
    class ReactorCore {
        public:
            ReactorCore();
            ~ReactorCore();
            void submit(Reaction task);
            reactionId_t getCurrentEventId(std::thread::id threadId);
        private:
            std::map<std::thread::id, ExecutionCore> cores;
            ReactorTaskQueue<Reaction> queue;
    };
}

#endif
