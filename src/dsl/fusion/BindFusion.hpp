/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include "../../threading/ReactionHandle.hpp"
#include "../../util/tuplify.hpp"
#include "../operation/DSLProxy.hpp"
#include "has_bind.hpp"

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

            /**
             * @brief This struct is used if there is a return type. It just passes the returned data back up.
             *
             * @return the data that is returned by the bind call
             */
            struct Return {
                template <typename... Arguments>
                static inline auto call(const std::shared_ptr<threading::Reaction>& reaction, Arguments&&... args) {
                    return Function::template bind<DSL>(reaction, std::forward<Arguments>(args)...);
                }
            };

            /**
             * @brief This struct is used if the return type of the bind function is void. It wraps it into an empty
             * tuple instead.
             *
             * @return an empty tuple
             */
            struct NoReturn {
                template <typename... Arguments>
                static inline std::tuple<> call(const std::shared_ptr<threading::Reaction>& reaction,
                                                Arguments&&... args) {
                    Function::template bind<DSL>(reaction, std::forward<Arguments>(args)...);
                    return std::tuple<>();
                }
            };


            template <typename... Arguments>
            static inline auto call(const std::shared_ptr<threading::Reaction>& reaction, Arguments&&... args)
                -> decltype(
                    std::conditional_t<std::is_void<decltype(Function::template bind<
                                                             DSL>(reaction, std::forward<Arguments>(args)...))>::value,
                                       NoReturn,
                                       Return>::template call(reaction, std::forward<Arguments>(args)...)) {

                return std::conditional_t<std::is_void<decltype(Function::template bind<DSL>(
                                              reaction, std::forward<Arguments>(args)...))>::value,
                                          NoReturn,
                                          Return>::template call(reaction, std::forward<Arguments>(args)...);
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

            template <typename DSL, typename... Arguments>
            static inline auto bind(const std::shared_ptr<threading::Reaction>& reaction, Arguments&&... args)
                -> decltype(
                    util::FunctionFusion<std::tuple<Word1, WordN...>,
                                         decltype(std::forward_as_tuple(reaction, std::forward<Arguments>(args)...)),
                                         BindCaller,
                                         std::tuple<DSL>,
                                         1>::call(reaction, std::forward<Arguments>(args)...)) {

                // Perform our function fusion
                return util::FunctionFusion<std::tuple<Word1, WordN...>,
                                            decltype(std::forward_as_tuple(reaction, std::forward<Arguments>(args)...)),
                                            BindCaller,
                                            std::tuple<DSL>,
                                            1>::call(reaction, std::forward<Arguments>(args)...);
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
