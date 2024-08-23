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

#ifndef NUCLEAR_DSL_FUSION_POSTCONDITION_FUSION_HPP
#define NUCLEAR_DSL_FUSION_POSTCONDITION_FUSION_HPP

#include "../../threading/ReactionTask.hpp"
#include "../../util/meta/Filter.hpp"
#include "../operation/DSLProxy.hpp"
#include "has_postcondition.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Redirect types without a postcondition function to their proxy type
        template <typename Word>
        using Postcondition = std::conditional_t<has_postcondition<Word>::value, Word, operation::DSLProxy<Word>>;

        /// Filter down DSL words to only those that have a postcondition function
        template <typename... Words>
        using PostconditionWords = Filter<has_postcondition, Postcondition<Words>...>;

        // Default case where there are no postcondition words
        template <typename Words>
        struct PostconditionFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct PostconditionFuser<std::tuple<Word>> {

            template <typename DSL>
            static void postcondition(threading::ReactionTask& task) {

                // Run our remaining postcondition
                Word::template postcondition<DSL>(task);
            }
        };

        // Case where there is more 2 more more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct PostconditionFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static void postcondition(threading::ReactionTask& task) {

                // Run our postcondition
                Word1::template postcondition<DSL>(task);

                // Run the rest of our postconditions
                PostconditionFuser<std::tuple<Word2, WordN...>>::template postcondition<DSL>(task);
            }
        };

        template <typename Word1, typename... WordN>
        struct PostconditionFusion : PostconditionFuser<PostconditionWords<Word1, WordN...>> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_POSTCONDITION_FUSION_HPP
