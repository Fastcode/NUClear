#include "ReactorCore.h"
#include <iostream>

namespace NUClear {
    
    ReactorCore::ReactorCore() : cores(10, ExecutionCore(queue)) {
    }
    
    ReactorCore::~ReactorCore() {
        for(auto core = std::begin(cores); core != std::end(cores); ++core) {
            cores[core]->kill();
        }
    }
    
    void ReactorCore::submit(Reaction task) {
        // Add the task to the queue
        queue.submit(task);
    }
    
    eventId_t ReactorCore::getCurrentEventId(std::thread::id threadId) {
        return -1;
    }
}
