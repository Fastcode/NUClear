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

#ifndef NUCLEAR_THREADING_REACTIONOPTIONS_H
#define NUCLEAR_THREADING_REACTIONOPTIONS_H

#include <memory>

#include "nuclear_bits/dsl/Priority.h"
#include "SyncQueue.h"

namespace NUClear {
    namespace threading {
        
        /**
         * @brief This struct holds all of the options which are specified in the Options<> part of a call.
         *
         * @details
         *  This struct is populated with the information which is specified in the Options section of a callback specifier.
         *  the options stored here influence how the scheduler handles this object (when it is scheduled in the thread pool)
         *
         * @author Trent Houliston
         */
        struct ReactionOptions {
            /**
             * @brief Creates a new ReactionOptions with the default options.
             *
             * @details
             *  This creates a new ReactionOptions with the default options. These default options are a Sync type of
             *  nullptr, a non single case, and DEFAULT priority.
             */
            ReactionOptions() : syncQueue(nullptr), single(false), priority(DEFAULT) {}
            
            /// @brief the sync type to use, acts as a compile time mutex
            std::shared_ptr<SyncQueue> syncQueue;
            /// @brief if only a single event of this type should be queued or running at any time
            bool single;
            /// @brief the priority with which to execute
            EPriority priority;
        };
    }
}
#endif
