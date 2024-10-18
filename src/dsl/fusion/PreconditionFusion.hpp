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

#ifndef NUCLEAR_DSL_FUSION_PRECONDITION_FUSION_HPP
#define NUCLEAR_DSL_FUSION_PRECONDITION_FUSION_HPP

#include "../../threading/ReactionTask.hpp"
#include "../operation/DSLProxy.hpp"
#include "FindWords.hpp"
#include "has_nuclear_dsl_method.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Make a SFINAE type to check if a word has a precondition method
        HAS_NUCLEAR_DSL_METHOD(precondition);

        // Default case where there are no precondition words
        template <typename Words>
        struct PreconditionFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct PreconditionFuser<std::tuple<Word>> {

            template <typename DSL>
            static bool precondition(threading::ReactionTask& task) {

                // Run our remaining precondition
                return Word::template precondition<DSL>(task);
            }
        };

        // Case where there is more 2 more more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct PreconditionFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static bool precondition(threading::ReactionTask& task) {

                // Perform a recursive and operation ending with the first false
                return Word1::template precondition<DSL>(task)
                       && PreconditionFuser<std::tuple<Word2, WordN...>>::template precondition<DSL>(task);
            }
        };

        template <typename Word1, typename... WordN>
        struct PreconditionFusion : PreconditionFuser<FindWords<has_precondition, Word1, WordN...>> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_PRECONDITION_FUSION_HPP
