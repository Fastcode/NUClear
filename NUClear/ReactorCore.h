#ifndef NUCLEAR_REACTORCORE_H
#define NUCLEAR_REACTORCORE_H

#include <functional>
#include <map>
#include <queue>
#include <typeindex>
#include <memory>
#include "Reaction.h"
#include "BlockingQueue.h"
#include "ExecutionCore.h"

namespace NUClear {
    class ReactorCore {
        public:
            ReactorCore();
            ~ReactorCore();
            void submit(std::unique_ptr<Reaction>&& reaction);
            void stop();
            void join();
            reactionId_t getCurrentReactionId(std::thread::id threadId);
        private:
            std::map<std::thread::id, std::unique_ptr<ExecutionCore>> cores;
            BlockingQueue<std::unique_ptr<Reaction>> queue;
    };
}

#endif
