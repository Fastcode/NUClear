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
    
    std::thread::id ExecutionCore::getThreadId() {
        return thread.get_id();
    }
    
    void ExecutionCore::core() {
        while(execute) {
            
            // Read the blocking queue for a task
            Reaction* t = nullptr;
            
            currentEventId = t->eventId;
            
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
