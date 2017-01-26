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

#ifndef NUCLEAR_DSL_FUSION_BINDFUSION_HPP
#define NUCLEAR_DSL_FUSION_BINDFUSION_HPP

#include "nuclear_bits/dsl/fusion/has_bind.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/threading/ReactionHandle.hpp"
#include "nuclear_bits/util/tuplify.hpp"

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
            template <typename Callback, typename... Arguments>
            static inline auto call(Reactor& reactor,
                                    const std::string& identifier,
                                    Callback&& callback,
                                    Arguments&&... args)
                -> decltype(Function::template bind<DSL>(reactor,
                                                         identifier,
                                                         std::forward<Callback>(callback),
                                                         std::forward<Arguments>(args)...)) {
                return Function::template bind<DSL>(
                    reactor, identifier, std::forward<Callback>(callback), std::forward<Arguments>(args)...);
            }
        };

        template <typename, typename = std::tuple<>>
        struct BindWords;

        /**
         * @brief Metafunction that extracts all of the Words with a bind function
         *
         * @tparam Word1        The word we are looking at
         * @tparam WordN        The words we have yet to look at
         * @tparam FoundWords   The words we have found with bind functions
         */
        template <typename Word1, typename... WordN, typename... FoundWords>
        struct BindWords<std::tuple<Word1, WordN...>, std::tuple<FoundWords...>>
            : public std::conditional_t<has_bind<Word1>::value,
                                        /*T*/ BindWords<std::tuple<WordN...>, std::tuple<FoundWords..., Word1>>,
                                        /*F*/ BindWords<std::tuple<WordN...>, std::tuple<FoundWords...>>> {};

        /**
         * @brief Termination case for the BindWords metafunction
         *
         * @tparam FoundWords The words we have found with bind functions
         */
        template <typename... FoundWords>
        struct BindWords<std::tuple<>, std::tuple<FoundWords...>> {
            using type = std::tuple<FoundWords...>;
        };

        /// Type that redirects types without a bind function to their proxy type
        template <typename Word>
        struct Bind {
            using type = std::conditional_t<has_bind<Word>::value, Word, operation::DSLProxy<Word>>;
        };

        // Default case where there are no bind words
        template <typename Words>
        struct BindFuser {};

        // Case where there is at least one bind word
        template <typename Word1, typename... WordN>
        struct BindFuser<std::tuple<Word1, WordN...>> {

            template <typename DSL, typename Function, typename... Arguments>
            static inline auto bind(Reactor& reactor,
                                    const std::string& identifier,
                                    Function&& callback,
                                    Arguments&&... args)
                -> decltype(util::FunctionFusion<std::tuple<Word1, WordN...>,
                                                 decltype(std::forward_as_tuple(reactor,
                                                                                identifier,
                                                                                std::forward<Function>(callback),
                                                                                std::forward<Arguments>(args)...)),
                                                 BindCaller,
                                                 std::tuple<DSL>,
                                                 3>::call(reactor,
                                                          identifier,
                                                          std::forward<Function>(callback),
                                                          std::forward<Arguments>(args)...)) {

                // Perform our function fusion
                return util::FunctionFusion<std::tuple<Word1, WordN...>,
                                            decltype(std::forward_as_tuple(reactor,
                                                                           identifier,
                                                                           std::forward<Function>(callback),
                                                                           std::forward<Arguments>(args)...)),
                                            BindCaller,
                                            std::tuple<DSL>,
                                            3>::call(reactor,
                                                     identifier,
                                                     std::forward<Function>(callback),
                                                     std::forward<Arguments>(args)...);
            }
        };

        template <typename Word1, typename... WordN>
        struct BindFusion
            : public BindFuser<
                  typename BindWords<std::tuple<typename Bind<Word1>::type, typename Bind<WordN>::type...>>::type> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_BINDFUSION_HPP
