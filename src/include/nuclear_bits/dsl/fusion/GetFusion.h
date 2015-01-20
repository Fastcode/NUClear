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

#ifndef NUCLEAR_DSL_FUSION_GETFUSION_H
#define NUCLEAR_DSL_FUSION_GETFUSION_H

#include "nuclear_bits/util/MetaProgramming.h"
#include "nuclear_bits/util/tuplify.h"
#include "nuclear_bits/threading/ReactionHandle.h"
#include "nuclear_bits/dsl/fusion/has_get.h"

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
            
            template <typename...>
            struct GetFusion;

            template <typename TFirst, typename... TWords>
            struct GetFusion<TFirst, TWords...> {
                
                template <typename DSL, typename U = TFirst>
                static inline auto get(threading::ReactionTask& task)
                -> EnableIf<All<Any<has_get<U>, has_get<operation::DSLProxy<U>>>, Any<Any<has_get<TWords>, has_get<operation::DSLProxy<TWords>>>...>>
                , decltype(std::tuple_cat(util::tuplify(If<has_get<U>, U, operation::DSLProxy<U>>::template get<DSL>(task)), If<All<Any<has_get<U>, has_get<operation::DSLProxy<U>>>, Any<Any<has_get<TWords>, has_get<operation::DSLProxy<TWords>>>...>>, GetFusion<TWords...>, NoOp>::template get<DSL>(task)))> {
                    // Tuplify and return what we need
                    
                    
                    
                    return std::tuple_cat(util::tuplify(If<has_get<U>, U, operation::DSLProxy<U>>::template get<DSL>(task)), If<All<Any<has_get<U>, has_get<operation::DSLProxy<U>>>, Any<Any<has_get<TWords>, has_get<operation::DSLProxy<TWords>>>...>>, GetFusion<TWords...>, NoOp>::template get<DSL>(task));
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto get(threading::ReactionTask& task)
                -> EnableIf<All<Any<has_get<U>, has_get<operation::DSLProxy<U>>>, Not<Any<Any<has_get<TWords>, has_get<operation::DSLProxy<TWords>>>...>>>
                , decltype(util::tuplify(If<has_get<U>, U, operation::DSLProxy<U>>::template get<DSL>(task)))> {
                    
                    // Tuplify and return our element
                    return util::tuplify(If<has_get<U>, U, operation::DSLProxy<U>>::template get<DSL>(task));
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto get(threading::ReactionTask& task)
                -> EnableIf<All<Not<Any<has_get<U>, has_get<operation::DSLProxy<U>>>>, Any<Any<has_get<TWords>, has_get<operation::DSLProxy<U>>>...>>
                , decltype(If<All<Not<Any<has_get<U>, has_get<operation::DSLProxy<U>>>>, Any<Any<has_get<TWords>, has_get<operation::DSLProxy<U>>>...>>, GetFusion<TWords...>, NoOp>::template get<DSL>(task))> {
                    
                    // Pass on to the next element
                    return GetFusion<TWords...>::template get<DSL>(task);
                }
            };
        }
    }
}

#endif
