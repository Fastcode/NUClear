/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_WORD_BUFFER_HPP
#define NUCLEAR_DSL_WORD_BUFFER_HPP

namespace NUClear {
    namespace dsl {
        namespace word {

            /**
             * @ingroup Options
             * @brief This option sets the number of tasks to buffer before dropping tasks
             *
             * @details
             *  If a task has a limited buffer size, then that means that N instances of the task can be
             *  in the system at any one time. If the task is triggered again while N existing tasks are in the
             *  queue or is still executing, then this new task will be ignored.
             */
            template <int N>
            struct Buffer {

                template <typename DSL>
                static inline bool precondition(threading::Reaction& reaction) {
                    // We only run if there are less than the target number of active tasks
                    return reaction.activeTasks < N;
                }
            };

        }  // namespace word
    }  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_BUFFER_HPP
