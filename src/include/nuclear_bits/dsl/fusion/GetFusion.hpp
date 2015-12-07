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

#include "nuclear_bits/util/MetaProgramming.hpp"
#include "nuclear_bits/util/tuplify.hpp"
#include "nuclear_bits/threading/ReactionHandle.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_get.hpp"

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
            struct GetCaller {
                static inline auto call(threading::Reaction& reaction)
                -> decltype(Function::template get<DSL>(reaction)) {
                    return Function::template get<DSL>(reaction);
                }
            };

            template<typename, typename = std::tuple<>>
            struct GetWords;

            /**
             * @brief Metafunction that extracts all of the Words with a get function
             *
             * @tparam TWord The word we are looking at
             * @tparam TRemainder The words we have yet to look at
             * @tparam TGetWords The words we have found with get functions
             */
            template <typename TWord, typename... TRemainder, typename... TGetWords>
            struct GetWords<std::tuple<TWord, TRemainder...>, std::tuple<TGetWords...>>
            : public std::conditional_t<has_get<TWord>::value,
                /*T*/ GetWords<std::tuple<TRemainder...>, std::tuple<TGetWords..., TWord>>,
                /*F*/ GetWords<std::tuple<TRemainder...>, std::tuple<TGetWords...>>> {};

            /**
             * @brief Termination case for the GetWords metafunction
             *
             * @tparam TGetWords The words we have found with get functions
             */
            template <typename... TGetWords>
            struct GetWords<std::tuple<>, std::tuple<TGetWords...>> {
                using type = std::tuple<TGetWords...>;
                static constexpr bool value = sizeof...(TGetWords) > 0;
            };

            template <typename TFirst, typename... TWords>
            struct GetFusion {

                // Get all of the words that have Get functionality, or the DSL proxy if it has one
                template <typename U>
                using Get = std::conditional_t<has_get<U>::value, U, operation::DSLProxy<U>>;

                template <typename DSL, typename U = TFirst>
                static inline auto get(threading::Reaction& reaction)
                -> std::enable_if_t<GetWords<std::tuple<Get<U>, Get<TWords>...>>::value
                , decltype(util::FunctionFusion<typename GetWords<std::tuple<Get<U>, Get<TWords>...>>::type
                           , decltype(std::forward_as_tuple(reaction))
                           , GetCaller
                           , std::tuple<DSL>
                           , 1>::call(reaction))> {
                    
                    // Perform our function fusion
                    return util::FunctionFusion<typename GetWords<std::tuple<Get<U>, Get<TWords>...>>::type
                    , decltype(std::forward_as_tuple(reaction))
                    , GetCaller
                    , std::tuple<DSL>
                    , 1>::call(reaction);
                }
            };
        }
    }
}

#endif
