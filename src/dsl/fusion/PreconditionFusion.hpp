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

#ifndef NUCLEAR_DSL_FUSION_PRECONDITIONFUSION_HPP
#define NUCLEAR_DSL_FUSION_PRECONDITIONFUSION_HPP

#include "../../threading/Reaction.hpp"
#include "../operation/DSLProxy.hpp"
#include "has_precondition.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Type that redirects types without a precondition function to their proxy type
        template <typename Word>
        struct Precondition {
            using type = std::conditional_t<has_precondition<Word>::value, Word, operation::DSLProxy<Word>>;
        };

        template <typename, typename = std::tuple<>>
        struct PreconditionWords;

        /**
         * @brief Metafunction that extracts all of the Words with a precondition function
         *
         * @tparam Word1        The word we are looking at
         * @tparam WordN        The words we have yet to look at
         * @tparam FoundWords   The words we have found with precondition functions
         */
        template <typename Word1, typename... WordN, typename... FoundWords>
        struct PreconditionWords<std::tuple<Word1, WordN...>, std::tuple<FoundWords...>>
            : public std::conditional_t<
                  has_precondition<typename Precondition<Word1>::type>::value,
                  /*T*/
                  PreconditionWords<std::tuple<WordN...>,
                                    std::tuple<FoundWords..., typename Precondition<Word1>::type>>,
                  /*F*/ PreconditionWords<std::tuple<WordN...>, std::tuple<FoundWords...>>> {};

        /**
         * @brief Termination case for the PreconditionWords metafunction
         *
         * @tparam FoundWords The words we have found with precondition functions
         */
        template <typename... FoundWords>
        struct PreconditionWords<std::tuple<>, std::tuple<FoundWords...>> {
            using type = std::tuple<FoundWords...>;
        };


        // Default case where there are no precondition words
        template <typename Words>
        struct PreconditionFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct PreconditionFuser<std::tuple<Word>> {

            template <typename DSL>
            static inline bool precondition(threading::Reaction& reaction) {

                // Run our remaining precondition
                return Word::template precondition<DSL>(reaction);
            }
        };

        // Case where there is more 2 more more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct PreconditionFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static inline bool precondition(threading::Reaction& reaction) {

                // Perform a recursive and operation ending with the first false
                return Word1::template precondition<DSL>(reaction)
                       && PreconditionFuser<std::tuple<Word2, WordN...>>::template precondition<DSL>(reaction);
            }
        };

        template <typename Word1, typename... WordN>
        struct PreconditionFusion
            : public PreconditionFuser<typename PreconditionWords<std::tuple<Word1, WordN...>>::type> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_PRECONDITIONFUSION_HPP
