#include "ReactorCore.h"
#include <iostream>

namespace NUClear {
    
    ReactorCore::ReactorCore() : cores(10) {
    }
    
    ReactorCore::~ReactorCore() {
        for(auto core = std::begin(cores); core != std::end(cores); ++core) {
            core->kill();
        }
    }
    
    void ReactorCore::submit(std::function<void ()> action) {
        // Add the action to the queue
    }
}
