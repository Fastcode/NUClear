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

#ifndef NUCLEAR_DSL_FUSION_BINDFUSION_H
#define NUCLEAR_DSL_FUSION_BINDFUSION_H

#include "nuclear_bits/util/MetaProgramming.h"
#include "nuclear_bits/util/tuplify.h"
#include "nuclear_bits/threading/ReactionHandle.h"
#include "nuclear_bits/dsl/operation/DSLProxy.h"
#include "nuclear_bits/dsl/fusion/has_bind.h"

namespace NUClear {
    namespace dsl {
        namespace fusion {
            
            // Import our metaprogramming tools
            template <typename Predecate, typename Then, typename Else>
            using If = util::Meta::If<Predecate, Then, Else>;
            
            template <typename Condition>
            using Not = util::Meta::Not<Condition>;
            
            template <typename... Conditions>
            using All = util::Meta::All<Conditions...>;
            
            template <typename... Conditions>
            using Any = util::Meta::Any<Conditions...>;
            
            // This function is needed because G++ is a little slow when it comes to templated functions and needs some help
            template <typename R, typename... A>
            auto resolve_function_type(R(*)(A...))
            -> R(*)(A...);
            
            /// Returns either the real type or the proxy if the real type does not have a bind function
            template <typename U>
            using Bind = If<has_bind<U>, U, operation::DSLProxy<U>>;
            
            template <typename...>
            struct BindFusion {
                template <typename DSL, typename TFunc>
                std::tuple<> bind(Reactor&, const std::string&, TFunc&&);
            };
            
            template <typename, typename, typename, typename, int>
            struct BindFission {};
            
            template <typename TFirst
            , typename... TWords
            , typename... TFirstArgs
            , typename... TWordsArgs>
            struct BindFission<TFirst, std::tuple<TWords...>, std::tuple<TFirstArgs...>, std::tuple<TWordsArgs...>, 1> {
                
                /**
                 * @brief This function is enabled in the situation that TFirst has a bind function, and there
                 *          is a bind function within TWords also.
                 *
                 * @tparam DSL      The parsed DSL that resulted from all these functions
                 * @tparam U        A copy of TFirst used for SFINAE
                 * @tparam TFunc    The type of the callback function passed to bind
                 * @tparam TArgs    The types of the runtime arguments passed into the bind
                 */
                template <typename DSL, typename TFunc>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc&& callback, TFirstArgs&&... ourArgs, TWordsArgs... otherArgs)
                -> decltype(std::tuple_cat(util::tuplify(Bind<TFirst>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TFirstArgs>(ourArgs)...))
                                        , BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TWordsArgs>(otherArgs)...))) {
                    
                    // Execute our function and then pass on the remainder
                    return std::tuple_cat(util::tuplify(Bind<TFirst>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TFirstArgs>(ourArgs)...))
                                          , BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TWordsArgs>(otherArgs)...));
                }
            };
            
            template <typename TFirst
            , typename... TWords
            , typename... TFirstArgs
            , typename... TWordsArgs>
            struct BindFission<TFirst, std::tuple<TWords...>, std::tuple<TFirstArgs...>, std::tuple<TWordsArgs...>, 2> {
                
                /**
                 * @brief This function is enabled in the situation that TFirst has a bind function, however there
                 *          are no further bind functions within our words
                 *
                 * @tparam DSL      The parsed DSL that resulted from all these functions
                 * @tparam U        A copy of TFirst used for SFINAE
                 * @tparam TFunc    The type of the callback function passed to bind
                 * @tparam TArgs    The types of the runtime arguments passed into the bind
                 */
                template <typename DSL, typename TFunc>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc&& callback, TFirstArgs&&... args)
                -> decltype(util::tuplify(Bind<TFirst>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TFirstArgs>(args)...))) {
                    
                    // Execute our function
                    return util::tuplify(Bind<TFirst>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TFirstArgs>(args)...));
                }
            };
            
            template <typename TFirst
            , typename... TWords
            , typename... TFirstArgs
            , typename... TWordsArgs>
            struct BindFission<TFirst, std::tuple<TWords...>, std::tuple<TFirstArgs...>, std::tuple<TWordsArgs...>, 3> {
                
                /**
                 * @brief This function is enabled in the situation that TFirst does not have a bind function, but there
                 *          are words further down that do
                 *
                 * @tparam DSL      The parsed DSL that resulted from all these functions
                 * @tparam U        A copy of TFirst used for SFINAE
                 * @tparam TFunc    The type of the callback function passed to bind
                 * @tparam TArgs    The types of the runtime arguments passed into the bind
                 */
                template <typename DSL, typename TFunc>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc&& callback, TWordsArgs&&... args)
                -> decltype(BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TWordsArgs>(args)...)) {
                    
                    // Forward onto the next stage of bind
                    return BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TWordsArgs>(args)...);
                }
            };
            
            /**
             *  @brief Performs an extraction of the arguments that make up the bind
             */
            template <typename>
            struct BindExtraction;
            
            template <typename R, typename I, typename Func, typename... TArgs>
            struct BindExtraction<std::tuple<R, I, Func, TArgs...>> {
                using type = std::tuple<TArgs...>;
            };
            
            template <typename TWord, int test = 0>
            struct BindArguments
            : public BindArguments<
              If<has_bind<TWord>, TWord, operation::DSLProxy<TWord>>,
              has_bind<If<has_bind<TWord>, TWord, operation::DSLProxy<TWord>>>::value ? 2 : 1> {
            };
            
            template <typename TWord>
            struct BindArguments<TWord, 1> {
                using type = std::tuple<>;
            };
            
            template <typename TWord>
            struct BindArguments<TWord, 2>
            : public BindExtraction<typename util::CallableInfo<decltype(resolve_function_type(TWord::template bind<ParsedNoOp, std::function<std::function<void()>(threading::ReactionTask&)>>))>::arguments> {
            };
            
            template <typename TFirst, typename... TWords>
            struct BindFusion<TFirst, TWords...>
            : public BindFission<
              TFirst
            , std::tuple<TWords...>
            , typename BindArguments<TFirst>::type
            , decltype(std::tuple_cat(std::declval<typename BindArguments<TWords>::type>()...))
            ,  All<has_bind<Bind<TFirst>>, Any<has_bind<Bind<TWords>>...>>::value      ? 1     // Us and our children
            :  All<has_bind<Bind<TFirst>>, Not<Any<has_bind<Bind<TWords>>...>>>::value ? 2     // Us but not our children
            :  All<Not<has_bind<Bind<TFirst>>>, Any<has_bind<Bind<TWords>>...>>::value ? 3 : 0 // Not us but our children
            > {};
        }
    }
}

#endif
