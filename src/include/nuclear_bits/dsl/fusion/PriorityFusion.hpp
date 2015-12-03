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

#ifndef NUCLEAR_DSL_FUSION_PRIORITYFUSION_H
#define NUCLEAR_DSL_FUSION_PRIORITYFUSION_H

#include "nuclear_bits/util/MetaProgramming.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_priority.hpp"

namespace NUClear {
    namespace dsl {
        namespace fusion {

            template <typename TFirst, typename... TWords>
            struct PriorityFusion {
            private:
                /// Returns either the real type or the proxy if the real type does not have a priority function
                template <typename U>
                using Priority = std::conditional_t<has_priority<U>::value, U, operation::DSLProxy<U>>;

                /// Checks if U has a priority function, and none of the following words do
                template <typename U>
                using UsNotChildren = All<has_priority<Priority<U>>, Not<Any<has_priority<Priority<TWords>>...>>>;

                /// Checks if we do not have a priority function, but at least one of the following words do
                template <typename U>
                using NotUsChildren = All<Not<has_priority<Priority<U>>>, Any<has_priority<Priority<TWords>>...>>;

            public:
                template <typename DSL, typename U = TFirst>
                static inline auto priority(threading::Reaction& task)
                -> std::enable_if_t<UsNotChildren<U>::value, int> {

                    // Return our priority
                    return Priority<U>::template priority<DSL>(task);
                }

                template <typename DSL, typename U = TFirst>
                static inline auto priority(threading::Reaction& task)
                -> std::enable_if_t<NotUsChildren<U>::value, int> {

                    // Get the priority from the future types
                    return PriorityFusion<TWords...>::template priority<DSL>(task);
                }
            };
        }
    }
}

#endif
