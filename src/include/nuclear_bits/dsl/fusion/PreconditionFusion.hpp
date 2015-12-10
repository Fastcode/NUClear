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

#ifndef NUCLEAR_DSL_FUSION_PRECONDITIONFUSION_H
#define NUCLEAR_DSL_FUSION_PRECONDITIONFUSION_H

#include "nuclear_bits/threading/Reaction.hpp"
#include "nuclear_bits/util/MetaProgramming.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_precondition.hpp"

namespace NUClear {
    namespace dsl {
        namespace fusion {

            /// Type that redirects types without a precondition function to their proxy type
            template <typename TWord>
            struct Precondition {
                using type = std::conditional_t<has_precondition<TWord>::value, TWord, operation::DSLProxy<TWord>>;
            };

            template<typename, typename = std::tuple<>>
            struct PreconditionWords;

            /**
             * @brief Metafunction that extracts all of the Words with a precondition function
             *
             * @tparam TWord The word we are looking at
             * @tparam TRemainder The words we have yet to look at
             * @tparam TPreconditionWords The words we have found with precondition functions
             */
            template <typename TWord, typename... TRemainder, typename... TPreconditionWords>
            struct PreconditionWords<std::tuple<TWord, TRemainder...>, std::tuple<TPreconditionWords...>>
            : public std::conditional_t<has_precondition<typename Precondition<TWord>::type>::value,
            /*T*/ PreconditionWords<std::tuple<TRemainder...>, std::tuple<TPreconditionWords..., typename Precondition<TWord>::type>>,
            /*F*/ PreconditionWords<std::tuple<TRemainder...>, std::tuple<TPreconditionWords...>>> {};

            /**
             * @brief Termination case for the PreconditionWords metafunction
             *
             * @tparam TPreconditionWords The words we have found with precondition functions
             */
            template <typename... TPreconditionWords>
            struct PreconditionWords<std::tuple<>, std::tuple<TPreconditionWords...>> {
                using type = std::tuple<TPreconditionWords...>;
            };


            // Default case where there are no precondition words
            template <typename TWords>
            struct PreconditionFuser {};

            // Case where there is only a single word remaining
            template <typename Word>
            struct PreconditionFuser<std::tuple<Word>> {

                template <typename DSL>
                static inline bool precondition(threading::Reaction& reaction) {
                    return Word::template precondition<DSL>(reaction);
                }
            };

            // Case where there is more 2 more more words remaining
            template <typename W1, typename W2, typename... WN>
            struct PreconditionFuser<std::tuple<W1, W2, WN...>> {

                template <typename DSL>
                static inline bool precondition(threading::Reaction& reaction) {
                    if(!W1::template precondition<DSL>(reaction)) {
                        return false;
                    }
                    else {
                        return PreconditionFuser<std::tuple<W2, WN...>>::template precondition<DSL>(reaction);
                    }
                }
            };

            template <typename W1, typename... WN>
            struct PreconditionFusion
            : public PreconditionFuser<typename PreconditionWords<std::tuple<W1, WN...>>::type> {
            };

        }
    }
}

#endif
