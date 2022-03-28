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

#ifndef NUCLEAR_DSL_FUSION_RESCHEDULEFUSION_HPP
#define NUCLEAR_DSL_FUSION_RESCHEDULEFUSION_HPP

#include "../../threading/ReactionTask.hpp"
#include "../operation/DSLProxy.hpp"
#include "has_reschedule.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Type that redirects types without a reschedule function to their proxy type
        template <typename Word>
        struct Reschedule {
            using type = std::conditional_t<has_reschedule<Word>::value, Word, operation::DSLProxy<Word>>;
        };

        template <typename, typename = std::tuple<>>
        struct RescheduleWords;

        /**
         * @brief Metafunction that extracts all of the Words with a reschedule function
         *
         * @tparam Word1        The word we are looking at
         * @tparam WordN        The words we have yet to look at
         * @tparam FoundWords   The words we have found with reschedule functions
         */
        template <typename Word1, typename... WordN, typename... FoundWords>
        struct RescheduleWords<std::tuple<Word1, WordN...>, std::tuple<FoundWords...>>
            : public std::conditional_t<
                  has_reschedule<typename Reschedule<Word1>::type>::value,
                  /*T*/
                  RescheduleWords<std::tuple<WordN...>, std::tuple<FoundWords..., typename Reschedule<Word1>::type>>,
                  /*F*/ RescheduleWords<std::tuple<WordN...>, std::tuple<FoundWords...>>> {};

        /**
         * @brief Termination case for the RescheduleWords metafunction
         *
         * @tparam FoundWords The words we have found with reschedule functions
         */
        template <typename... FoundWords>
        struct RescheduleWords<std::tuple<>, std::tuple<FoundWords...>> {
            using type = std::tuple<FoundWords...>;
        };


        // Default case where there are no reschedule words
        template <typename Words>
        struct RescheduleFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct RescheduleFuser<std::tuple<Word>> {

            template <typename DSL>
            static inline std::unique_ptr<threading::ReactionTask> reschedule(
                std::unique_ptr<threading::ReactionTask>&& task) {

                // Pass our task to see if it gets rescheduled and return the result
                return Word::template reschedule<DSL>(std::move(task));
            }
        };

        // Case where there is more 2 more more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct RescheduleFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static inline std::unique_ptr<threading::ReactionTask> reschedule(
                std::unique_ptr<threading::ReactionTask>&& task) {

                // Pass our task to see if it gets rescheduled
                auto ptr = Word1::template reschedule<DSL>(std::move(task));

                // If it was not rescheduled pass to the next rescheduler
                if (ptr) {
                    return RescheduleFuser<std::tuple<Word2, WordN...>>::template reschedule<DSL>(std::move(ptr));
                }
                else {
                    return ptr;
                }
            }
        };

        template <typename Word1, typename... WordN>
        struct RescheduleFusion : public RescheduleFuser<typename RescheduleWords<std::tuple<Word1, WordN...>>::type> {
        };

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_RESCHEDULEFUSION_HPP
