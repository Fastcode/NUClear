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

#ifndef NUCLEAR_DSL_WORD_SYNC_H
#define NUCLEAR_DSL_WORD_SYNC_H

namespace NUClear {
    namespace dsl {
        namespace word {

            /**
             * @ingroup Options
             * @brief This option sets the Synchronization group of the task
             *
             * @details
             *  The synchronization group of a task is a compile time mutex which will allow only a single task from
             *  each distinct execution task to execute at a time. For example, if two tasks both had Sync<int> then only
             *  one of those tasks would execute at a time.
             *
             * @tparam TSync the type with which to synchronize on
             */
            template <typename TSync>
            struct Sync {
                
                using task_ptr = std::unique_ptr<threading::ReactionTask>;
                
                /// @brief our queue which sorts tasks by priority
                static std::priority_queue<task_ptr> queue;
                static int running;
                static std::mutex mutex;
                
                template <typename DSL>
                static inline std::unique_ptr<threading::ReactionTask> reschedule(std::unique_ptr<threading::ReactionTask>&& task) {
                    
                    // Lock our mutex
                    std::lock_guard<std::mutex> lock(mutex);
                    
                    // One more running task!
                    ++running;
                    
                    // If this is our first task we don't sync
                    if(running == 1) {
                        return std::move(task);
                    }
                    // We are not the first, put our task in the queue
                    else {
                        queue.push(std::move(task));
                        return std::unique_ptr<threading::ReactionTask>(nullptr);
                    }
                }
                
                template <typename DSL>
                static void postcondition(threading::ReactionTask& task) {
                    
                    // Lock our mutex
                    std::lock_guard<std::mutex> lock(mutex);
                    
                    // One less running task!
                    --running;
                    
                    // If we have any tasks left over resubmit
                    if(!queue.empty()) {
                        
                        // Return the type
                        // If you're wondering why all the ridiculousness, it's because priority queue is not as feature complete as it should be
                        std::unique_ptr<threading::ReactionTask> nextTask(std::move(const_cast<std::unique_ptr<threading::ReactionTask>&>(queue.top())));
                        queue.pop();
                        
                        // Resubmit this task to the reaction queue
                        task.parent.reactor.powerplant.submit(std::move(nextTask));
                    }
                }
            };
            
            template <typename TSync>
            int Sync<TSync>::running = 0;
        }
    }
}

#endif
