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
            
            // Start our timer
            
            // Execute the callback from the blocking queue
            
            // If at any point the emit function is called, then store that events initiator as our intitiator
            
            //End our timer
        }
    }
}
