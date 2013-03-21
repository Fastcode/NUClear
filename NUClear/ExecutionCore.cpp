#include "ExecutionCore.h"

namespace NUClear {

    void swap(ExecutionCore& first, ExecutionCore& second) 
    {
        // If this looks weird to you check out the copy-and-swap idiom:
        // http://stackoverflow.com/a/3279550/203133
        using std::swap; 

        swap(first.execute, second.execute);
        swap(first.currentReactionId, second.currentReactionId);
        swap(first.thread, second.thread);
    }
    
    ExecutionCore::ExecutionCore(ReactionQueue& queue) :
        execute(true), 
        queue(queue), 
        thread(std::bind(&ExecutionCore::core, this)) {
    }

    ExecutionCore::ExecutionCore(ExecutionCore&& other) : 
        ExecutionCore(other.queue) {
        swap(*this, other);
    }

    ExecutionCore& ExecutionCore::operator=(ExecutionCore&& other) {
        swap(*this, other);
        return *this;
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
    
    reactionId_t ExecutionCore::getCurrentReactionId() {
        return currentReactionId;
    }
    
    void ExecutionCore::core() {
        while(execute) {
            
            // Read the blocking queue for a task         
            Reaction r = queue.dequeue();
            
            //Set our current event id we are processing
            currentReactionId = r.reactionId;
            
            // Get our start time
            r.startTime = std::time(nullptr);
            
            // Execute the callback from the blocking queue
            r.callback();
            
            // Get our end time
            r.endTime = std::time(nullptr);
            
            // Our task is finished, here is where any details surounding the statistics of the task can be processed
        }
    }
}
