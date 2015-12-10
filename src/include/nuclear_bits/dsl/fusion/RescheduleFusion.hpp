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

#ifndef NUCLEAR_DSL_FUSION_RESCHEDULEFUSION_H
#define NUCLEAR_DSL_FUSION_RESCHEDULEFUSION_H

#include "nuclear_bits/threading/ReactionTask.hpp"
#include "nuclear_bits/util/MetaProgramming.hpp"
#include "nuclear_bits/dsl/operation/DSLProxy.hpp"
#include "nuclear_bits/dsl/fusion/has_reschedule.hpp"

namespace NUClear {
    namespace dsl {
        namespace fusion {

            /// Type that redirects types without a reschedule function to their proxy type
            template <typename TWord>
            struct Reschedule {
                using type = std::conditional_t<has_reschedule<TWord>::value, TWord, operation::DSLProxy<TWord>>;
            };

            template<typename, typename = std::tuple<>>
            struct RescheduleWords;

            /**
             * @brief Metafunction that extracts all of the Words with a reschedule function
             *
             * @tparam TWord The word we are looking at
             * @tparam TRemainder The words we have yet to look at
             * @tparam TRescheduleWords The words we have found with reschedule functions
             */
            template <typename TWord, typename... TRemainder, typename... TRescheduleWords>
            struct RescheduleWords<std::tuple<TWord, TRemainder...>, std::tuple<TRescheduleWords...>>
            : public std::conditional_t<has_reschedule<typename Reschedule<TWord>::type>::value,
            /*T*/ RescheduleWords<std::tuple<TRemainder...>, std::tuple<TRescheduleWords..., typename Reschedule<TWord>::type>>,
            /*F*/ RescheduleWords<std::tuple<TRemainder...>, std::tuple<TRescheduleWords...>>> {};

            /**
             * @brief Termination case for the RescheduleWords metafunction
             *
             * @tparam TRescheduleWords The words we have found with reschedule functions
             */
            template <typename... TRescheduleWords>
            struct RescheduleWords<std::tuple<>, std::tuple<TRescheduleWords...>> {
                using type = std::tuple<TRescheduleWords...>;
            };


            // Default case where there are no reschedule words
            template <typename TWords>
            struct RescheduleFuser {};

            // Case where there is only a single word remaining
            template <typename Word>
            struct RescheduleFuser<std::tuple<Word>> {

                template <typename DSL>
                static inline std::unique_ptr<threading::ReactionTask> reschedule(std::unique_ptr<threading::ReactionTask>&& task) {

                    // Pass our task to see if it gets rescheduled and return the result
                    return Word::template reschedule<DSL>(std::move(task));
                }
            };

            // Case where there is more 2 more more words remaining
            template <typename W1, typename W2, typename... WN>
            struct RescheduleFuser<std::tuple<W1, W2, WN...>> {

                template <typename DSL>
                static inline std::unique_ptr<threading::ReactionTask> reschedule(std::unique_ptr<threading::ReactionTask>&& task) {

                    // Pass our task to see if it gets rescheduled
                    auto ptr = W1::template rescheudle<DSL>(std::move(task));

                    // If it was not rescheduled pass to the next rescheduler
                    if(ptr) {
                        return RescheduleFuser<std::tuple<W2, WN...>>::template reschedule<DSL>(std::move(ptr));
                    }
                    else {
                        return ptr;
                    }
                }
            };

            template <typename W1, typename... WN>
            struct RescheduleFusion
            : public RescheduleFuser<typename RescheduleWords<std::tuple<W1, WN...>>::type> {
            };

        }
    }
}

#endif
