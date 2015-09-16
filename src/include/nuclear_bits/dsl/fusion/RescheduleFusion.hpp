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

#ifndef NUCLEAR_DSL_FUSION_RESCHEDULEFUSION_H
#define NUCLEAR_DSL_FUSION_RESCHEDULEFUSION_H

#include "nuclear_bits/util/MetaProgramming.hpp"

#include "nuclear_bits/threading/ReactionTask.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_reschedule.hpp"

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
            struct RescheduleFusion {
            private:
                /// Returns either the real type or the proxy if the real type does not have a reschedule function
                template <typename U>
                using Reschedule = If<has_reschedule<U>, U, operation::DSLProxy<U>>;
                
                /// Checks if U has a reschedule function, and at least one of the following words do
                template <typename U>
                using UsAndChildren = All<has_reschedule<Reschedule<U>>, Any<has_reschedule<Reschedule<TWords>>...>>;
                
                /// Checks if U has a reschedule function, and none of the following words do
                template <typename U>
                using UsNotChildren = All<has_reschedule<Reschedule<U>>, Not<Any<has_reschedule<Reschedule<TWords>>...>>>;
                
                /// Checks if we do not have a reschedule function, but at least one of the following words do
                template <typename U>
                using NotUsChildren = All<Not<has_reschedule<Reschedule<U>>>, Any<has_reschedule<Reschedule<TWords>>...>>;

                
            public:
                template <typename DSL, typename U = TFirst>
                static inline auto reschedule(std::unique_ptr<threading::ReactionTask>&& task)
                -> EnableIf<UsAndChildren<U>, std::unique_ptr<threading::ReactionTask>> {
                    
                    // Run this reschedule
                    auto ptr = Reschedule<U>::template reschedule<DSL>(std::move(task));
                    
                    // If we have more reschedules (this reschedule didn't take it)
                    if(ptr) {
                        // Run future reschedules
                        RescheduleFusion<TWords...>::template reschedule<DSL>(task);
                    }
                    else {
                        return std::move(ptr);
                    }
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto reschedule(std::unique_ptr<threading::ReactionTask>&& task)
                -> EnableIf<UsNotChildren<U>, std::unique_ptr<threading::ReactionTask>> {
                    
                    // If we haven't already been rescheduled
                    if(task) {
                        // Run this reschedule
                        return U::template reschedule<DSL>(std::move(task));
                    }
                    else {
                        // Just return our task
                        return std::move(task);
                    }
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto reschedule(std::unique_ptr<threading::ReactionTask>&& task)
                -> EnableIf<NotUsChildren<U>, std::unique_ptr<threading::ReactionTask>> {
                    
                    // Run future reschedules
                    return RescheduleFusion<TWords...>::template reschedule<DSL>(std::move(task));
                }
            };
        }
    }
}

#endif
