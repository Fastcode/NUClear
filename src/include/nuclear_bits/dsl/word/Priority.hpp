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

#ifndef NUCLEAR_DSL_WORD_PRIORITY_HPP
#define NUCLEAR_DSL_WORD_PRIORITY_HPP

#include "nuclear_bits/threading/Reaction.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief Used to control the priority of the response of the callback
         *
         * @details
         *  This is designed to assign priority and determine the scheduling order in the threadpool and also the
         *  prority of the thread @code on<Trigger<T, ...>, Priority::HIGH>() @endcode
         *
         * @par TRENT????
         *  Flagged for more discussion. I am getting to this - I just skipped over this particiular one today.
         */
        struct Priority {

            struct REALTIME {
                /// Realtime priority runs with 1000 value
                static constexpr int value = 1000;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };

            struct HIGH {
                /// High priority runs with 750 value
                static constexpr int value = 750;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };

            struct NORMAL {
                /// Normal priority runs with 500 value
                static constexpr int value = 500;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };

            struct LOW {
                /// Low priority runs with 250 value
                static constexpr int value = 250;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };

            struct IDLE {
                /// Idle tasks run with 0 priority, they run when there is free time
                static constexpr int value = 0;

                template <typename DSL>
                static inline int priority(threading::Reaction&) {
                    return value;
                }
            };
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_PRIORITY_HPP
