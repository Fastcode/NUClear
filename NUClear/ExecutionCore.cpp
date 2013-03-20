#include "ExecutionCore.h"

namespace NUClear {
    
    ExecutionCore::ExecutionCore() : execute(true), thread(std::bind(&ExecutionCore::core, this)) {
    }
    
    ExecutionCore::~ExecutionCore() {
        execute = false;
        thread.detach();
    }
    
    void ExecutionCore::kill() {
        execute = false;
    }
    
    void ExecutionCore::core() {
        while(execute) {
            
            // Read the blocking queue for a task
            
            // Store what the initiator was for this execution
            
            // Get our start time
            std::time_t x = std::time(nullptr);
            
            // Execute the callback from the blocking queue
            
            // Get our end time
            std::time_t y = std::time(nullptr);
            
            // If at any point the emit function is called, then store that events initiator as our intitiator
            
            //End our timer
        }
    }
}
