#include "ExecutionCore.h"

namespace NUClear {
    
    ExecutionCore::ExecutionCore(ReactorTaskQueue<Reaction>& queue) : execute(true), queue(queue), thread(std::bind(&ExecutionCore::core, this)) {
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
            Reaction* t = nullptr;
            
            // Get our start time
            t->startTime = std::time(nullptr);
            
            // Execute the callback from the blocking queue
            t->callback();
            
            // Get our end time
            t->endTime = std::time(nullptr);
            
            // Our task is finished, here is where any details surounding the statistics of the task can be processed
        }
    }
}
