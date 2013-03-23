#include "TimeEmitter.h"

namespace NUClear {
    
    TimeEmitter::TimeEmitter() : execute(true), thread(std::bind(&TimeEmitter::run, this)) {
    }
    
    TimeEmitter::~TimeEmitter() {
        execute = false;
        thread.detach();
    }
    
    void TimeEmitter::add(std::chrono::nanoseconds nanos, std::function<void ()> emit) {
        
    }
    
    void TimeEmitter::run() {
        
        while(execute) {
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            // Sleep for however many nanoseconds we need until our next event emition
            
            // Run the callback for however many events mod our time are there
        }
    }
}