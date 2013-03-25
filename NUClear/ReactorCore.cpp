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
        shutdown();
    }
    
    void ReactorCore::submit(std::unique_ptr<Reaction>&& reaction) {
        // Add the task to the queue
        queue.push(std::move(reaction));
    }

    void ReactorCore::shutdown() {
        std::cerr << "Killing core start" << std::endl;
        for(auto it = std::begin(cores); it != std::end(cores); ++it) {
            it->second->kill();
        }
        std::cerr << "Killing done" << std::endl;
        queue.stop();
    }

    void ReactorCore::waitForThreadCompletion() {
        std::cerr << "Joining waitFor" << std::endl;
        for(auto it = std::begin(cores); it != std::end(cores); ++it) {
            std::cerr << "Joining" << std::endl;
            it->second->join();
        }
        std::cerr << "Joining waitFor done" << std::endl;
    }
    
    reactionId_t ReactorCore::getCurrentReactionId(std::thread::id threadId) {
        return cores.count(threadId) ? cores[threadId]->getCurrentReactionId() : -1;
    }
}
