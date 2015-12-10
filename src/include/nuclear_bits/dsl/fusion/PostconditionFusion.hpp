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

#ifndef NUCLEAR_DSL_FUSION_POSTCONDITIONFUSION_H
#define NUCLEAR_DSL_FUSION_POSTCONDITIONFUSION_H

#include "nuclear_bits/threading/ReactionTask.hpp"
#include "nuclear_bits/util/MetaProgramming.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_postcondition.hpp"

namespace NUClear {
    namespace dsl {
        namespace fusion {

            /// Type that redirects types without a postcondition function to their proxy type
            template <typename TWord>
            struct Postcondition {
                using type = std::conditional_t<has_postcondition<TWord>::value, TWord, operation::DSLProxy<TWord>>;
            };

            template<typename, typename = std::tuple<>>
            struct PostconditionWords;

            /**
             * @brief Metafunction that extracts all of the Words with a postcondition function
             *
             * @tparam TWord The word we are looking at
             * @tparam TRemainder The words we have yet to look at
             * @tparam TPostconditionWords The words we have found with postcondition functions
             */
            template <typename TWord, typename... TRemainder, typename... TPostconditionWords>
            struct PostconditionWords<std::tuple<TWord, TRemainder...>, std::tuple<TPostconditionWords...>>
            : public std::conditional_t<has_postcondition<typename Postcondition<TWord>::type>::value,
            /*T*/ PostconditionWords<std::tuple<TRemainder...>, std::tuple<TPostconditionWords..., typename Postcondition<TWord>::type>>,
            /*F*/ PostconditionWords<std::tuple<TRemainder...>, std::tuple<TPostconditionWords...>>> {};

            /**
             * @brief Termination case for the PostconditionWords metafunction
             *
             * @tparam TPostconditionWords The words we have found with postcondition functions
             */
            template <typename... TPostconditionWords>
            struct PostconditionWords<std::tuple<>, std::tuple<TPostconditionWords...>> {
                using type = std::tuple<TPostconditionWords...>;
            };


            // Default case where there are no postcondition words
            template <typename TWords>
            struct PostconditionFuser {};

            // Case where there is only a single word remaining
            template <typename Word>
            struct PostconditionFuser<std::tuple<Word>> {

                template <typename DSL>
                static inline void postcondition(threading::ReactionTask& task) {

                    // Run our postcondition
                    Word::template postcondition<DSL>(task);
                }
            };

            // Case where there is more 2 more more words remaining
            template <typename W1, typename W2, typename... WN>
            struct PostconditionFuser<std::tuple<W1, W2, WN...>> {

                template <typename DSL>
                static inline void postcondition(threading::ReactionTask& task) {

                    // Run our postcondition
                    W1::template postcondition<DSL>(task);

                    // Run the rest of our postconditions
                    PostconditionFuser<std::tuple<W2, WN...>>::template postcondition<DSL>(task);
                }
            };

            template <typename W1, typename... WN>
            struct PostconditionFusion
            : public PostconditionFuser<typename PostconditionWords<std::tuple<W1, WN...>>::type> {
            };

        }
    }
}

#endif
