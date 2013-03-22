#include "ReactorCore.h"
#include <iostream>

namespace NUClear {
    
    ReactorCore::ReactorCore() {
        int numCores = 10;
        for(int i = 0; i < numCores; ++i) {
            std::unique_ptr<ExecutionCore> core(new ExecutionCore(queue));
            cores[core->getThreadId()] = std::move(core);
        }
    }
    
    ReactorCore::~ReactorCore() {
        for(auto it = std::begin(cores); it != std::end(cores); ++it) {
            it->second->kill();
        }
    }
    
    void ReactorCore::submit(Reaction task) {
        // Add the task to the queue
        queue.enqueue(task);
    }
    
    reactionId_t ReactorCore::getCurrentReactionId(std::thread::id threadId) {
        return cores.count(threadId) ? cores[threadId]->getCurrentReactionId() : -1;
    }
}
