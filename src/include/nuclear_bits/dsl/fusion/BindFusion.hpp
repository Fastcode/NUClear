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

#include "nuclear_bits/util/MetaProgramming.hpp"
#include "nuclear_bits/util/tuplify.hpp"
#include "nuclear_bits/threading/ReactionHandle.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_bind.hpp"

namespace NUClear {
    namespace dsl {
        namespace fusion {

            /**
             * @brief This is our Function Fusion wrapper class that allows it to call bind functions
             *
             * @tparam Function the bind function that we are wrapping for
             * @tparam DSL      the DSL that we pass to our bind function
             */
            template <typename Function, typename DSL>
            struct BindCaller {
                template <typename TFunc, typename... TArgs>
                static inline auto call(Reactor& reactor, const std::string& identifier, TFunc&& callback, TArgs&&... args)
                -> decltype(Function::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...)) {
                    return Function::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...);
                }
            };

            template<typename, typename = std::tuple<>>
            struct BindWords;

            /**
             * @brief Metafunction that extracts all of the Words with a bind function
             *
             * @tparam TWord The word we are looking at
             * @tparam TRemainder The words we have yet to look at
             * @tparam TBindWords The words we have found with bind functions
             */
            template <typename TWord, typename... TRemainder, typename... TBindWords>
            struct BindWords<std::tuple<TWord, TRemainder...>, std::tuple<TBindWords...>>
            : public std::conditional_t<has_bind<TWord>::value,
                /*T*/ BindWords<std::tuple<TRemainder...>, std::tuple<TBindWords..., TWord>>,
                /*F*/ BindWords<std::tuple<TRemainder...>, std::tuple<TBindWords...>>> {};

            /**
             * @brief Termination case for the BindWords metafunction
             *
             * @tparam TBindWords The words we have found with bind functions
             */
            template <typename... TBindWords>
            struct BindWords<std::tuple<>, std::tuple<TBindWords...>> {
                using type = std::tuple<TBindWords...>;
            };
            
            /// Type that redirects types without a bind function to their proxy type
            template <typename TWord>
            struct Bind {
                using type = std::conditional_t<has_bind<TWord>::value, TWord, operation::DSLProxy<TWord>>;
            };
            
            // Default case where there are no bind words
            template <typename TWords>
            struct BindFuser {};
            
            // Case where there is at least one bind word
            template <typename TFirst, typename... TWords>
            struct BindFuser<std::tuple<TFirst, TWords...>> {
                template <typename DSL, typename TFunc, typename... TArgs>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc&& callback, TArgs&&... args)
                -> decltype(util::FunctionFusion<std::tuple<TFirst, TWords...>
                           , decltype(std::forward_as_tuple(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...))
                           , BindCaller
                           , std::tuple<DSL>
                           , 3>::call(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...)) {
                    
                    // Perform our function fusion
                    return util::FunctionFusion<std::tuple<TFirst, TWords...>
                    , decltype(std::forward_as_tuple(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...))
                    , BindCaller
                    , std::tuple<DSL>
                    , 3>::call(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...);
                }
            };
            
            template <typename TFirst, typename... TWords>
            struct BindFusion
            : public BindFuser<typename BindWords<std::tuple<typename Bind<TFirst>::type, typename Bind<TWords>::type...>>::type> {
            };
        }
    }
}

#endif
