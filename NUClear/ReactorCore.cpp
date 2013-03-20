#include "ReactorCore.h"
#include <iostream>

namespace NUClear {
    
    ReactorCore::ReactorCore() : cores(10) {
    }
    
    ReactorCore::~ReactorCore() {
        for(int i = 0; i < cores.size(); ++i) {
            cores[i].kill();
        }
    }
    
    void ReactorCore::submit(std::function<void ()> action) {
        // Add the action to the queue
    }
}
