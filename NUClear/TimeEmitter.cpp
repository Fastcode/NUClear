#include "TimeEmitter.h"

namespace NUClear {
    
    TimeEmitter::TimeEmitter() : thread(std::bind(&TimeEmitter::run, this)) {
    }
    
    TimeEmitter::~TimeEmitter() {
        thread.detach();
    }
    
    void TimeEmitter::add(int millis, std::function<void ()> emit) {
        
    }
    
    void TimeEmitter::run() {
        
        // Sleep for however many nanoseconds we need until our next event emition
        
        // Run the callback for however many events mod our time are there
    }
}