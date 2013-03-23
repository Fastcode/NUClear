#include "ReactorCore.h"
#include <iostream>

namespace NUClear {
    
    ReactorCore::ReactorCore() {
        std::cerr << "ReactorCore constructor start" << std::endl;
        int numCores = 10;
        for(int i = 0; i < numCores; ++i) {
            std::unique_ptr<ExecutionCore> core(new ExecutionCore(queue));
            cores[core->getThreadId()] = std::move(core);
        }
        std::cerr << "ReactorCore constructor end" << std::endl;
    }
    
    ReactorCore::~ReactorCore() {
        std::cerr << "~ReactorCore()" << std::endl;
        stop();
    }
    
    void ReactorCore::submit(std::unique_ptr<Reaction>&& reaction) {
        // Add the task to the queue
        queue.push(std::move(reaction));
    }

    void ReactorCore::stop() {
        for(auto it = std::begin(cores); it != std::end(cores); ++it) {
            it->second->kill();
        }
        queue.stop();
    }

    void ReactorCore::join() {
        for(auto it = std::begin(cores); it != std::end(cores); ++it) {
            it->second->join();
        }
    }
    
    reactionId_t ReactorCore::getCurrentReactionId(std::thread::id threadId) {
        return cores.count(threadId) ? cores[threadId]->getCurrentReactionId() : -1;
    }
}
