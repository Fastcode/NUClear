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

#ifndef NUCLEAR_DSL_FUSION_PRECONDITIONFUSION_H
#define NUCLEAR_DSL_FUSION_PRECONDITIONFUSION_H

#include "nuclear_bits/util/MetaProgramming.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_precondition.hpp"

namespace NUClear {
    namespace dsl {
        namespace fusion {
            
            template <typename Predecate, typename Then, typename Else>
            using If = util::Meta::If<Predecate, Then, Else>;
            
            template <typename Condition, typename Value>
            using EnableIf = util::Meta::EnableIf<Condition, Value>;
            
            template <typename Condition>
            using Not = util::Meta::Not<Condition>;
            
            template <typename... Conditions>
            using All = util::Meta::All<Conditions...>;
            
            template <typename... Conditions>
            using Any = util::Meta::Any<Conditions...>;
            
            template <typename TFirst, typename... TWords>
            struct PreconditionFusion {
            private:
                /// Returns either the real type or the proxy if the real type does not have a precondition function
                template <typename U>
                using Precondition = If<has_precondition<U>, U, operation::DSLProxy<U>>;
                
                /// Checks if U has a precondition function, and at least one of the following words do
                template <typename U>
                using UsAndChildren = All<has_precondition<Precondition<U>>, Any<has_precondition<Precondition<TWords>>...>>;
                
                /// Checks if U has a precondition function, and none of the following words do
                template <typename U>
                using UsNotChildren = All<has_precondition<Precondition<U>>, Not<Any<has_precondition<Precondition<TWords>>...>>>;
                
                /// Checks if we do not have a precondition function, but at least one of the following words do
                template <typename U>
                using NotUsChildren = All<Not<has_precondition<Precondition<U>>>, Any<has_precondition<Precondition<TWords>>...>>;
            
            public:
                template <typename DSL, typename U = TFirst>
                static inline auto precondition(threading::Reaction& task)
                -> EnableIf<UsAndChildren<U>, bool> {
                    
                    // Run this precondition
                    if(!Precondition<U>::template precondition<DSL>(task)) {
                        return false;
                    }
                    // Run future preconditions
                    else {
                        return PreconditionFusion<TWords...>::template precondition<DSL>(task);
                    }
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto precondition(threading::Reaction& task)
                -> EnableIf<UsNotChildren<U>, bool> {
                    
                    // Run this precondition
                    return Precondition<U>::template precondition<DSL>(task);
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto precondition(threading::Reaction& task)
                -> EnableIf<NotUsChildren<U>, bool> {
                    
                    // Run future precondition
                    return PreconditionFusion<TWords...>::template precondition<DSL>(task);
                }
            };
        }
    }
}

#endif
