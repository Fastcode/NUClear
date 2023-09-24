/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#ifndef NUCLEAR_DSL_FUSION_GROUPFUSION_HPP
#define NUCLEAR_DSL_FUSION_GROUPFUSION_HPP

#include <algorithm>
#include <stdexcept>

#include "../../threading/Reaction.hpp"
#include "../operation/DSLProxy.hpp"
#include "has_group.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Type that redirects types without a group function to their proxy type
        template <typename Word>
        struct Group {
            using type = std::conditional_t<has_group<Word>::value, Word, operation::DSLProxy<Word>>;
        };

        template <typename, typename = std::tuple<>>
        struct GroupWords;

        /**
         * @brief Metafunction that extracts all of the Words with a group function
         *
         * @tparam Word1        The word we are looking at
         * @tparam WordN        The words we have yet to look at
         * @tparam FoundWords   The words we have found with group functions
         */
        template <typename Word1, typename... WordN, typename... FoundWords>
        struct GroupWords<std::tuple<Word1, WordN...>, std::tuple<FoundWords...>>
            : public std::conditional_t<
                  has_group<typename Group<Word1>::type>::value,
                  /*T*/ GroupWords<std::tuple<WordN...>, std::tuple<FoundWords..., typename Group<Word1>::type>>,
                  /*F*/ GroupWords<std::tuple<WordN...>, std::tuple<FoundWords...>>> {};

        /**
         * @brief Termination case for the GroupWords metafunction
         *
         * @tparam FoundWords The words we have found with group functions
         */
        template <typename... FoundWords>
        struct GroupWords<std::tuple<>, std::tuple<FoundWords...>> {
            using type = std::tuple<FoundWords...>;
        };


        // Default case where there are no group words
        template <typename Words>
        struct GroupFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct GroupFuser<std::tuple<Word>> {

            template <typename DSL>
            static inline util::GroupDescriptor group(threading::Reaction& reaction) {

                // Return our group
                return Word::template group<DSL>(reaction);
            }
        };

        // Case where there are 2 or more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct GroupFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static inline void group(const threading::Reaction& /*reaction*/) {
                throw std::invalid_argument("Can not be a member of more than one group");
            }
        };

        template <typename Word1, typename... WordN>
        struct GroupFusion : public GroupFuser<typename GroupWords<std::tuple<Word1, WordN...>>::type> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_GROUPFUSION_HPP
