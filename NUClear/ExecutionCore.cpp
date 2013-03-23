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
    
    ExecutionCore::ExecutionCore(BlockingQueue<std::unique_ptr<Reaction>>& queue) :
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
        std::cerr << "~ExecutionCore()" << std::endl;
        execute = false;
    }
    
    void ExecutionCore::kill() {
        execute = false;
    }

    void ExecutionCore::join() {
        thread.join();
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
            std::unique_ptr<Reaction> r = queue.pop();
            
            // Set our current event id we are processing
            currentReactionId = r->reactionId;
            
            // Get our start time
            r->startTime = std::chrono::steady_clock::now();
            
            // Execute the callback from the blocking queue
            r->callback();
            
            // Get our end time
            r->endTime = std::chrono::steady_clock::now();
            
            // Our task is finished, here is where any details surounding the statistics of the task can be processed
        }
    }
}
