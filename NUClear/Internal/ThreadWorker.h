/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NUCLEAR_THREADWORKER_H
#define NUCLEAR_THREADWORKER_H

#include <thread>
#include "Reaction.h"
#include "TaskScheduler.h"

namespace NUClear {
namespace Internal {
    
    /**
     * @brief This class implements a wrapper around an std::thread to act as a member of a thread pool.
     * 
     * @details
     *  The thread worker class is responsible for executing Reactions which come in to be executed. It gets these
     *  tasks from the TaskSchedulers TaskScheduler, and then executes these tasks.
     *
     * @author Trent Houliston
     */
    class ThreadWorker {
        public:
            /**
             * @brief Constructs a new ThreadWorker using the passed TaskScheduler to get tasks.
             *
             * @param scheduler the scheduler to get tasks to process from while this thread is running
             */
            ThreadWorker(TaskScheduler* scheduler);
        
            /**
             * @brief Constructs a new ThreadWorker which executes the passed task until it's execute is false
             *
             * @param task the task to execute repeatedly until execute is false
             */
            ThreadWorker(std::function<void ()> task);
        
            /**
             * @brief destructs this ThreadWorker instance, it will kill the thread if it is not already dead (should
             *  always be already dead)
             */
            ~ThreadWorker();
        
            /**
             * @brief Gets the thread id of the thread that this object holds.
             *
             * @return the thread id of the thread this object contains
             */
            std::thread::id getThreadId();
        
            /**
             * @brief Tells this thread to stop executing as soon as it has finished executing it's current task.
             *
             * @details
             *  This method will tell the pool thread that after it has finished executing it's current task, it should
             *  not take any more tasks and end it's execution. This method is run when the system shuts down.
             */
            void kill();
        
            /**
             * @brief Waits until this thread has finished executing and then returns.
             *
             * @details
             *  This method will wait until the thread object which is executing internally finishes its execution and
             *  terminates. It is called when the system is shutting down by the main thread. This ensures that the main
             *  thread does not terminate until all of the pool threads (which may be using static resources) are finished.
             *
             *  This is important as once the main thread finishes execution it will start deleting all static objects,
             *  and then terminate execution. So to prevent this, we wait until everything else is finished before ending
             *  the main thread.
             */
            void join();
        
            /**
             * @brief Gets the reaction that is currently executing in this thread object.
             *
             * @details
             *  This method gets the ReactionTask object that this thread is executing. It should only ever be called
             *  by this thread itself from within an emit function. This allows the system to build a call tree of events
             *  as the created event will know the Reaction which it was created from.
             *
             * @return the ReactionTask object that this thread is currently processing
             */
            Reaction::Task& getCurrentReaction();
        
        private:
            /**
             * @brief Method which is executed by the thread when it starts up.
             * 
             * @details
             *  This method is the method that the thread will execute when it starts up. This method is simply a loop
             *  which gets a task from the TaskScheduler and executes it until the execute variable is false.
             */
            void core();
        
            /// @brief the task scheduler that tasks are collected from
            TaskScheduler* m_scheduler;
            /// @brief the internal task that will be executed without a scheduler
            std::function<void ()> m_task;
            /// @brief a variable used to check if we should still run
            volatile bool m_execute;
            /// @brief a pointer to the current reaction we are working on
            Reaction::Task* m_currentReaction;
            /// @brief our internal thread object
            std::thread m_thread;
    };
}
}

#endif
