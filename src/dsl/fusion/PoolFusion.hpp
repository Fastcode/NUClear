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

#ifndef NUCLEAR_DSL_FUSION_POOLFUSION_HPP
#define NUCLEAR_DSL_FUSION_POOLFUSION_HPP

#include <algorithm>
#include <stdexcept>

#include "../../threading/Reaction.hpp"
#include "../operation/DSLProxy.hpp"
#include "has_pool.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Type that redirects types without a pool function to their proxy type
        template <typename Word>
        struct Pool {
            using type = std::conditional_t<has_pool<Word>::value, Word, operation::DSLProxy<Word>>;
        };

        template <typename, typename = std::tuple<>>
        struct PoolWords;

        /**
         * @brief Metafunction that extracts all of the Words with a pool function
         *
         * @tparam Word1        The word we are looking at
         * @tparam WordN        The words we have yet to look at
         * @tparam FoundWords   The words we have found with pool functions
         */
        template <typename Word1, typename... WordN, typename... FoundWords>
        struct PoolWords<std::tuple<Word1, WordN...>, std::tuple<FoundWords...>>
            : public std::conditional_t<
                  has_pool<typename Pool<Word1>::type>::value,
                  /*T*/ PoolWords<std::tuple<WordN...>, std::tuple<FoundWords..., typename Pool<Word1>::type>>,
                  /*F*/ PoolWords<std::tuple<WordN...>, std::tuple<FoundWords...>>> {};

        /**
         * @brief Termination case for the PoolWords metafunction
         *
         * @tparam FoundWords The words we have found with pool functions
         */
        template <typename... FoundWords>
        struct PoolWords<std::tuple<>, std::tuple<FoundWords...>> {
            using type = std::tuple<FoundWords...>;
        };


        // Default case where there are no pool words
        template <typename Words>
        struct PoolFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct PoolFuser<std::tuple<Word>> {

            template <typename DSL>
            static inline util::ThreadPoolDescriptor pool(threading::Reaction& reaction) {

                // Return our pool
                return Word::template pool<DSL>(reaction);
            }
        };

        // Case where there are 2 or more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct PoolFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static inline util::ThreadPoolDescriptor pool(const threading::Reaction& /*reaction*/) {
                throw std::invalid_argument("Can not be a member of more than one pool");
            }
        };

        template <typename Word1, typename... WordN>
        struct PoolFusion : public PoolFuser<typename PoolWords<std::tuple<Word1, WordN...>>::type> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_POOLFUSION_HPP
