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

#ifndef NUCLEAR_THREADING_SYNCQUEUE_H
#define NUCLEAR_THREADING_SYNCQUEUE_H

#include <typeindex>
#include <queue>
#include <vector>
#include <mutex>
#include <memory>
#include <string>

#include "ReactionTask.h"

namespace NUClear {
    namespace threading {
        
        /**
         * @brief Holds tasks outside of the main thread queue to prevent multiple execution.
         *
         * @details
         *  This queue is used in order to store tasks that have identical sync types.
         *  This prevents multiple tasks with the same type from running at once.
         *  It also stores if a task from the queue is currently active.
         *
         * @author Trent Houliston
         */
        struct SyncQueue {
            
            /**
             * @brief Construct a new SyncQueue for the passed type.
             *
             * @param type the type that this sync queue is managing.
             */
            SyncQueue(const std::type_index type) : type(type), active(false) {}
            
            /// @brief the type that this sync queue is managing
            const std::type_index type;
            
            /// @brief if a task from this queue is currently in the main queue
            volatile bool active;
            
            /// @brief a mutex to protect access to the queue
            std::mutex mutex;
            
            /// @brief a priority queue that stores overflow tasks
            std::priority_queue<std::unique_ptr<ReactionTask>> queue;
        };
        
        /**
         * @brief This holds a static pointer to a queue that is to be used for a type
         *
         * @tparam QueueFor the type that this queue is for
         *
         * @author Trent Houliston
         */
        template <typename QueueFor>
        struct SyncQueueFor {
            static std::shared_ptr<SyncQueue> queue;
        };
        
        // Build a queue for each type we use
        template <typename QueueFor>
        std::shared_ptr<SyncQueue> SyncQueueFor<QueueFor>::queue(std::make_shared<SyncQueue>(typeid(QueueFor)));
    }
}
#endif
