/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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

#ifndef NUCLEAR_DSL_FUSION_BIND_FUSION_HPP
#define NUCLEAR_DSL_FUSION_BIND_FUSION_HPP

#include "../../threading/Reaction.hpp"
#include "../../util/FunctionFusion.hpp"
#include "../operation/DSLProxy.hpp"
#include "FindWords.hpp"
#include "has_nuclear_dsl_method.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Make a SFINAE type to check if a word has a run_inline method
        HAS_NUCLEAR_DSL_METHOD(bind);

        /**
         * This is our Function Fusion wrapper class that allows it to call bind functions.
         *
         * @tparam Function the bind function that we are wrapping for
         * @tparam DSL      the DSL that we pass to our bind function
         */
        template <typename Function, typename DSL>
        struct BindCaller {

            /**
             * This struct is used if there is a return type.
             * It just passes the returned data back up.
             *
             * @return the data that is returned by the bind call
             */
            struct Return {
                template <typename... Arguments>
                static auto call(const std::shared_ptr<threading::Reaction>& reaction, Arguments&&... args) {
                    return Function::template bind<DSL>(reaction, std::forward<Arguments>(args)...);
                }
            };

            /**
             * This struct is used if the return type of the bind function is void.
             * It wraps it into an empty tuple instead.
             *
             * @return an empty tuple
             */
            struct NoReturn {
                template <typename... Arguments>
                static std::tuple<> call(const std::shared_ptr<threading::Reaction>& reaction, Arguments&&... args) {
                    Function::template bind<DSL>(reaction, std::forward<Arguments>(args)...);
                    return {};
                }
            };


            template <typename... Arguments>
            static auto call(const std::shared_ptr<threading::Reaction>& reaction, Arguments&&... args)
                -> decltype(std::conditional_t<std::is_void<decltype(Function::template bind<DSL>(
                                                   reaction,
                                                   std::forward<Arguments>(args)...))>::value,
                                               NoReturn,
                                               Return>::template call(reaction, std::forward<Arguments>(args)...)) {

                return std::conditional_t<
                    std::is_void<decltype(Function::template bind<DSL>(reaction,
                                                                       std::forward<Arguments>(args)...))>::value,
                    NoReturn,
                    Return>::template call(reaction, std::forward<Arguments>(args)...);
            }
        };

        // Default case where there are no bind words
        template <typename Words>
        struct BindFuser {};

        // Case where there is at least one bind word
        template <typename Word1, typename... WordN>
        struct BindFuser<std::tuple<Word1, WordN...>> {

            template <typename DSL, typename... Arguments>
            static auto bind(const std::shared_ptr<threading::Reaction>& reaction, Arguments&&... args)
                -> decltype(util::FunctionFusion<std::tuple<Word1, WordN...>,
                                                 decltype(std::forward_as_tuple(reaction,
                                                                                std::forward<Arguments>(args)...)),
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
        struct BindFusion : BindFuser<FindWords<has_bind, Word1, WordN...>> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_BIND_FUSION_HPP
