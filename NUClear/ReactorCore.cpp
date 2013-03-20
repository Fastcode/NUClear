#include "ReactorCore.h"
#include <iostream>

namespace NUClear {
    
    ReactorCore::ReactorCore() : cores(10, ReactorCore(queue)) {
    }
    
    ReactorCore::~ReactorCore() {
        for(auto core = std::begin(cores); core != std::end(cores); ++core) {
            core->kill();
        }
    }
    
    void ReactorCore::submit(Reaction task) {
        // Add the task to the queue
        queue.submit(task);
    }
}
