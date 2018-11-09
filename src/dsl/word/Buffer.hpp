/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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
         * @brief
         *  This is used to specify that up to n instances of the associated reaction can execute during runtime.
         *
         * @details
         *  @code on<Trigger<T, ...>, Buffer<n>>>() @endcode
         *  In the case above, when the subscribing reaction is triggered, should there be less than <i>n</i> existing
         *  tasks associated with this reaction (either executing or in the queue), then a new task will be created and
         *  scheduled.  However, should <i>n</i> tasks already be allocated, then this new task request will be ignored.
         *
         *  For best use, this word should be fused with at least one other binding DSL word.
         *
         * @par Implements
         *  Precondition, Fusion
         *
         * @tparam n
         *  the number of tasks (instances of the subscribing reaction) which can be running at a given time.
         */
        template <int n>
        struct Buffer {

            template <typename DSL>
            static inline bool precondition(threading::Reaction& reaction) {
                // We only run if there are less than the target number of active tasks
                return reaction.active_tasks < (n + 1);
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_BUFFER_HPP
