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

#ifndef NUCLEAR_THREADING_REACTION_H
#define NUCLEAR_THREADING_REACTION_H

#include <functional>
#include <atomic>
#include <memory>
#include <string>

#include "ReactionOptions.h"
#include "ReactionTask.h"

namespace NUClear {
    namespace threading {
        
        /**
         * @brief This class holds the definition of a Reaction (call signature).
         *
         * @details
         *  A reaction holds the information about a callback. It holds the options as to how to process it in the scheduler.
         *  It also holds a function which is used to generate databound Task objects (callback with the function arguments
         *  already loaded and ready to run).
         *
         * @author Trent Houliston
         */
        class Reaction {
            // Reaction handles are given to user code to enable and disable the reaction
            friend class ReactionHandle;
            
        public:
            /**
             * @brief Constructs a new Reaction with the passed callback generator and options
             *
             * @param name      string identifier information about the reaction to help identify it
             * @param callback  the callback generator function (creates databound callbacks)
             * @param options   the options to use in Tasks
             */
            Reaction(std::string name, std::function<std::function<void (ReactionTask&)> ()> callback, ReactionOptions options);
            
            /**
             * @brief creates a new databound callback task that can be executed.
             *
             * @param cause the task that caused this task to run (or nullptr if did not have a parent)
             *
             * @return a unique_ptr to a Task which has the data for it's call bound into it
             */
            std::unique_ptr<ReactionTask> getTask(const ReactionTask* cause);
            
            /**
             * @brief returns true if this reaction is currently enabled
             */
            bool isEnabled();
            
            /// @brief This holds the demangled name of the On function that is being called
            const std::string name;
            /// @brief the options for this Reaction (decides how Tasks will be scheduled)
            ReactionOptions options;
            /// @brief the unique identifier for this Reaction object
            const uint64_t reactionId;
            /// @brief if this reaction is currently enqueued or running
            volatile bool running;
        private:
            /// @brief a source for reactionIds, atomically creates longs
            static std::atomic<uint64_t> reactionIdSource;
            /// @brief if this reaction object is currently enabled
            std::atomic<bool> enabled;
            /// @brief the callback generator function (creates databound callbacks)
            std::function<std::function<void (ReactionTask&)> ()> callback;
        };
    }
}
#endif
