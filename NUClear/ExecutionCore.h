#ifndef NUCLEAR_EXECUTIONCORE_H
#define NUCLEAR_EXECUTIONCORE_H

#include <thread>
#include <ctime>
#include "Reaction.h"
#include "ReactorTaskQueue.h"

namespace NUClear {
    class ExecutionCore {
    public:
        ExecutionCore(ReactorTaskQueue<Reaction>& queue);
        ExecutionCore(const ExecutionCore& other) = delete;
        ExecutionCore& operator=(const ExecutionCore& other) = delete;
        ExecutionCore(ExecutionCore&& other);
        ExecutionCore& operator=(ExecutionCore&& other);
        friend void swap(ExecutionCore& first, ExecutionCore& second);

        ~ExecutionCore();

        std::thread::id getThreadId();

        void kill();
    private:
        void core();

        bool execute;
        reactionId_t currentEventId;
        std::thread thread;
        ReactorTaskQueue<Reaction>& queue;
    };
}

#endif
