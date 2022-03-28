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

#ifndef NUCLEAR_DSL_FUSION_POSTCONDITIONFUSION_HPP
#define NUCLEAR_DSL_FUSION_POSTCONDITIONFUSION_HPP

#include "../../threading/ReactionTask.hpp"
#include "../operation/DSLProxy.hpp"
#include "has_postcondition.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Type that redirects types without a postcondition function to their proxy type
        template <typename Word>
        struct Postcondition {
            using type = std::conditional_t<has_postcondition<Word>::value, Word, operation::DSLProxy<Word>>;
        };

        template <typename, typename = std::tuple<>>
        struct PostconditionWords;

        /**
         * @brief Metafunction that extracts all of the Words with a postcondition function
         *
         * @tparam Word1        The word we are looking at
         * @tparam WordN        The words we have yet to look at
         * @tparam FoundWords   The words we have found with postcondition functions
         */
        template <typename Word1, typename... WordN, typename... FoundWords>
        struct PostconditionWords<std::tuple<Word1, WordN...>, std::tuple<FoundWords...>>
            : public std::conditional_t<
                  has_postcondition<typename Postcondition<Word1>::type>::value,
                  /*T*/
                  PostconditionWords<std::tuple<WordN...>,
                                     std::tuple<FoundWords..., typename Postcondition<Word1>::type>>,
                  /*F*/ PostconditionWords<std::tuple<WordN...>, std::tuple<FoundWords...>>> {};

        /**
         * @brief Termination case for the PostconditionWords metafunction
         *
         * @tparam PostconditionWords The words we have found with postcondition functions
         */
        template <typename... FoundWords>
        struct PostconditionWords<std::tuple<>, std::tuple<FoundWords...>> {
            using type = std::tuple<FoundWords...>;
        };


        // Default case where there are no postcondition words
        template <typename Words>
        struct PostconditionFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct PostconditionFuser<std::tuple<Word>> {

            template <typename DSL>
            static inline void postcondition(threading::ReactionTask& task) {

                // Run our remaining postcondition
                Word::template postcondition<DSL>(task);
            }
        };

        // Case where there is more 2 more more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct PostconditionFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static inline void postcondition(threading::ReactionTask& task) {

                // Run our postcondition
                Word1::template postcondition<DSL>(task);

                // Run the rest of our postconditions
                PostconditionFuser<std::tuple<Word2, WordN...>>::template postcondition<DSL>(task);
            }
        };

        template <typename Word1, typename... WordN>
        struct PostconditionFusion
            : public PostconditionFuser<typename PostconditionWords<std::tuple<Word1, WordN...>>::type> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_POSTCONDITIONFUSION_HPP
