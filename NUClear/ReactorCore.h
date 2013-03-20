#ifndef NUCLEAR_REACTORCORE_H
#define NUCLEAR_REACTORCORE_H

#include <functional>
#include <vector>
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
            int getCause();
        private:
            std::vector<ExecutionCore> cores;
            ReactorTaskQueue<Reaction> queue;
    };
}

#endif