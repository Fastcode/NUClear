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

#ifndef NUCLEAR_DSL_FUSION_GET_FUSION_HPP
#define NUCLEAR_DSL_FUSION_GET_FUSION_HPP

#include "../../threading/Reaction.hpp"
#include "../../util/meta/Filter.hpp"
#include "../../util/tuplify.hpp"
#include "../operation/DSLProxy.hpp"
#include "has_get.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /**
         * This is our Function Fusion wrapper class that allows it to call get functions.
         *
         * @tparam Function the get function that we are wrapping for
         * @tparam DSL      the DSL that we pass to our get function
         */
        template <typename Function, typename DSL>
        struct GetCaller {
            static auto call(threading::ReactionTask& task) -> decltype(Function::template get<DSL>(task)) {
                return Function::template get<DSL>(task);
            }
        };

        /// Redirect types without a get function to their proxy type
        template <typename Word>
        using Get = std::conditional_t<has_get<Word>::value, Word, operation::DSLProxy<Word>>;

        /// Filter down DSL words to only those that have a get function
        template <typename... Words>
        using GetWords = Filter<has_get, Get<Words>...>;

        // Default case where there are no get words
        template <typename Words>
        struct GetFuser {};

        // Case where there is at least one get word
        template <typename Word1, typename... WordN>
        struct GetFuser<std::tuple<Word1, WordN...>> {

            template <typename DSL, typename U = Word1>
            static auto get(threading::ReactionTask& task)
                -> decltype(util::FunctionFusion<std::tuple<Word1, WordN...>,
                                                 decltype(std::forward_as_tuple(task)),
                                                 GetCaller,
                                                 std::tuple<DSL>,
                                                 1>::call(task)) {

                // Perform our function fusion
                return util::FunctionFusion<std::tuple<Word1, WordN...>,
                                            decltype(std::forward_as_tuple(task)),
                                            GetCaller,
                                            std::tuple<DSL>,
                                            1>::call(task);
            }
        };

        template <typename Word1, typename... WordN>
        struct GetFusion : GetFuser<GetWords<Word1, WordN...>> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_GET_FUSION_HPP
