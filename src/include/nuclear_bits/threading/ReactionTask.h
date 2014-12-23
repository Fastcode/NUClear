/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_THREADING_REACTIONTASK_H
#define NUCLEAR_THREADING_REACTIONTASK_H

#include <functional>
#include <atomic>
#include <vector>
#include <typeindex>
#include <memory>

#include "nuclear_bits/ReactionStatistics.h"

namespace NUClear {
    namespace threading {
        // Forward declare reaction
        class Reaction;
        
        /**
         * @brief This is a databound call of a Reaction ready to be executed.
         *
         * @details
         *  This class holds a reaction that is ready to be executed. It is a Reaction object which has had it's callback
         *  parameters bound with data. This can then be executed as a function to run the call inside it.
         *
         * @author Trent Houliston
         */
        class ReactionTask {
        private:
            /// @brief a source for reactionIds, atomically creates longs
            static std::atomic<std::uint64_t> taskIdSource;
        public:
            /**
             * @brief Creates a new ReactionTask object bound with the parent Reaction object (that created it) and task.
             *
             * @param parent    the Reaction object that spawned this ReactionTask.
             * @param cause     the task that caused this task to run.
             * @param task      the data bound callback to be executed in the threadpool.
             */
            ReactionTask(Reaction* parent, const ReactionTask* cause, std::function<void ()> task);
            
            /**
             * @brief Runs the internal data bound task and times it.
             *
             * @details
             *  This runs the internal data bound task and times how long the execution takes. These figures can then be
             *  used in a debugging context to calculate how long callbacks are taking to run.
             */
            void operator()();
            
            /// @brief the data bound callback to be executed
            std::function<void ()> callback;
            /// @brief the parent Reaction object which spawned this
            Reaction* parent;
            /// @brief the taskId of this task (the sequence number of this paticular task)
            uint64_t taskId;
            /// @brief the statistics object that persists after this for information and debugging
            std::unique_ptr<ReactionStatistics> stats;
            
            static __thread ReactionTask* currentTask;
        };
    }
}
#endif
