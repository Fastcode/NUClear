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

#ifndef NUCLEAR_DSL_FUSION_POSTCONDITIONFUSION_H
#define NUCLEAR_DSL_FUSION_POSTCONDITIONFUSION_H

#include "nuclear_bits/threading/ReactionHandle.h"

namespace NUClear {
    namespace dsl {
        namespace fusion {
            
            template <typename Condition, typename Value>
            using EnableIf = util::Meta::EnableIf<Condition, Value>;
            
            template <typename Condition>
            using Not = util::Meta::Not<Condition>;
            
            template <typename... Conditions>
            using All = util::Meta::All<Conditions...>;
            
            template <typename... Conditions>
            using Any = util::Meta::Any<Conditions...>;
            
            // SFINAE for testing the existence of the operational functions in a type
            namespace {
                
                template<typename T>
                struct has_postcondition {
                private:
                    typedef std::true_type yes;
                    typedef std::false_type no;
                    
                    template<typename U> static auto test(int) -> decltype(U::template postcondition<void>(std::declval<threading::ReactionTask&>()), yes());
                    template<typename> static no test(...);
                    
                public:
                    static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
                };
            }
            
            
            template <typename TFirst, typename... TWords>
            struct PostconditionFusion {
                
                template <typename DSL, typename U = TFirst>
                static inline auto postcondition(threading::ReactionTask& task)
                -> EnableIf<All<has_postcondition<U>, Any<has_postcondition<TWords>...>>, void> {
                    
                    // Run this postcondition
                    TFirst::template postcondition<DSL>(task);
                    
                    // Run future postcondition
                    PreconditionFusion<TWords...>::template postcondition<DSL>(task);
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto postcondition(threading::ReactionTask& task)
                -> EnableIf<All<has_postcondition<U>, Not<Any<has_postcondition<TWords>...>>>, void> {
                    
                    // Run this postcondition
                    TFirst::template postcondition<DSL>(task);
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto postcondition(threading::ReactionTask& task)
                -> EnableIf<All<Not<has_postcondition<U>>, Any<has_postcondition<TWords>...>>, void> {
                    
                    // Run future postcondition
                    PreconditionFusion<TWords...>::template postcondition<DSL>(task);
                }
            };
        }
    }
}

#endif
