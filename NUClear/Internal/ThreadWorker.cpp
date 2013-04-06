#include "ThreadWorker.h"

namespace NUClear {
namespace Internal {
    
    ThreadWorker::ThreadWorker() :
    execute(true),
    currentReaction(nullptr),
    thread(std::bind(&ThreadWorker::core, this)) {
    }
    
    ThreadWorker::~ThreadWorker() {
        execute = false;
    }
    
    void ThreadWorker::kill() {
        execute = false;
    }
    
    void ThreadWorker::join() {
        if(thread.joinable()) {
            thread.join();
        }
    }
    
    std::thread::id ThreadWorker::getThreadId() {
        return thread.get_id();
    }
    
    Reaction& ThreadWorker::getCurrentReaction() {
        return *currentReaction;
    }
    
    void ThreadWorker::core() {
        while(execute) {
            
        }
    }
}
}
