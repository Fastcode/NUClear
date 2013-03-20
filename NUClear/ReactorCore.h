#ifndef NUCLEAR_REACTORCORE_H
#define NUCLEAR_REACTORCORE_H

#include <functional>
#include <vector>
#include <queue>
#include "ExecutionCore.h"

namespace NUClear {
    class ReactorCore {
        public:
            ReactorCore();
            ~ReactorCore();
            void submit(std::function<void ()>);
            int getCause();
        private:
            std::vector<ExecutionCore> cores;
            std::queue<std::function<void ()>> tasks;
    };
}

#endif