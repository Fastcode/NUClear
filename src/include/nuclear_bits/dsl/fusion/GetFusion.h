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
                struct has_get {
                private:
                    typedef std::true_type yes;
                    typedef std::false_type no;
                    
                    template<typename U> static auto test(int) -> decltype(U::template get<void>(std::declval<threading::ReactionTask&>()), yes());
                    template<typename> static no test(...);
                    
                public:
                    static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
                };
                
                template <typename TData>
                static inline std::tuple<TData> tuplify(TData&& data) {
                    return std::make_tuple(std::move(data));
                }
                
                template <typename... TElements>
                static inline std::tuple<TElements...> tuplify(std::tuple<TElements...>&& tuple) {
                    return tuple;
                }
            }
            
            template <typename TFirst, typename... TWords>
            struct GetFusion {
                
                
                template <typename DSL, typename U = TFirst>
                static inline auto get(threading::ReactionTask& task)
                -> EnableIf<All<has_get<U>, Any<has_get<TWords>...>>
                , decltype(std::tuple_cat(tuplify(TFirst::template get<DSL>(task)), GetFusion<TWords...>::template get<DSL>(task)))> {
                    
                    // Tuplify and return what we need
                    return std::tuple_cat(tuplify(TFirst::template get<DSL>(task)), GetFusion<TWords...>::template get<DSL>(task));
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto get(threading::ReactionTask& task)
                -> EnableIf<All<has_get<U>, Not<Any<has_get<TWords>...>>>
                , decltype(tuplify(TFirst::template get<DSL>(task)))> {
                    
                    // Tuplify and return our element
                    return tuplify(TFirst::template get<DSL>(task));
                }
                
                template <typename DSL, typename U = TFirst>
                static inline auto get(threading::ReactionTask& task)
                -> EnableIf<All<Not<has_get<U>>, Any<has_get<TWords>...>>
                , decltype(GetFusion<TWords...>::template get<DSL>(task))> {
                    
                    // Pass on to the next element
                    return GetFusion<TWords...>::template get<DSL>(task);
                }
                
                // Has + futures
                // Has - futures
                // NoHas + futures
                
                
                // IDEA
                // pass down a tuple of "Active Getters" and when we reach the "no further getters" then we make a tuple from all of them at the end of the line
                
                
                
                
//                template <typename DSL>
//                static auto get(threading::ReactionTask& task) -> decltype(std::tuple_cat((Tuplify<decltype(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::template get<DSL>(std::forward<threading::ReactionTask&>(task)))>::make(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::template get<DSL>(std::forward<threading::ReactionTask&>(task))))...)) {
//                    
//                    return std::tuple_cat((Tuplify<decltype(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::template get<DSL>(std::forward<threading::ReactionTask&>(task)))>::make(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::template get<DSL>(std::forward<threading::ReactionTask&>(task))))...);
//                }
            };
        }
    }
}

#endif
