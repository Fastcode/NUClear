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

#ifndef NUCLEAR_DSL_FUSION_PRIORITYFUSION_H
#define NUCLEAR_DSL_FUSION_PRIORITYFUSION_H

#include "nuclear_bits/threading/Reaction.hpp"
#include "nuclear_bits/util/MetaProgramming.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_priority.hpp"

namespace NUClear {
    namespace dsl {
        namespace fusion {

            /// Type that redirects types without a priority function to their proxy type
            template <typename TWord>
            struct Priority {
                using type = std::conditional_t<has_priority<TWord>::value, TWord, operation::DSLProxy<TWord>>;
            };

            template<typename, typename = std::tuple<>>
            struct PriorityWords;

            /**
             * @brief Metafunction that extracts all of the Words with a priority function
             *
             * @tparam TWord The word we are looking at
             * @tparam TRemainder The words we have yet to look at
             * @tparam TPriorityWords The words we have found with priority functions
             */
            template <typename TWord, typename... TRemainder, typename... TPriorityWords>
            struct PriorityWords<std::tuple<TWord, TRemainder...>, std::tuple<TPriorityWords...>>
            : public std::conditional_t<has_priority<typename Priority<TWord>::type>::value,
            /*T*/ PriorityWords<std::tuple<TRemainder...>, std::tuple<TPriorityWords..., typename Priority<TWord>::type>>,
            /*F*/ PriorityWords<std::tuple<TRemainder...>, std::tuple<TPriorityWords...>>> {};

            /**
             * @brief Termination case for the PriorityWords metafunction
             *
             * @tparam TPriorityWords The words we have found with priority functions
             */
            template <typename... TPriorityWords>
            struct PriorityWords<std::tuple<>, std::tuple<TPriorityWords...>> {
                using type = std::tuple<TPriorityWords...>;
            };


            // Default case where there are no priority words
            template <typename TWords>
            struct PriorityFuser {};

            // Case where there is only a single word remaining
            template <typename Word>
            struct PriorityFuser<std::tuple<Word>> {

                template <typename DSL>
                static inline int priority(threading::Reaction& reaction) {

                    // Return our priority
                    return Word::template priority<DSL>(reaction);
                }
            };

            // Case where there is more 2 more more words remaining
            template <typename W1, typename W2, typename... WN>
            struct PriorityFuser<std::tuple<W1, W2, WN...>> {

                template <typename DSL>
                static inline int priority(threading::Reaction& reaction) {

                    // Choose our maximum priority
                    return std::max(W1::template priority<DSL>(reaction),
                                    PriorityFuser<std::tuple<W2, WN...>>::template priority<DSL>(reaction));
                }
            };

            template <typename W1, typename... WN>
            struct PriorityFusion
            : public PriorityFuser<typename PriorityWords<std::tuple<W1, WN...>>::type> {
            };

        }
    }
}

#endif
