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

#ifndef NUCLEAR_DSL_FUSION_POOL_FUSION_HPP
#define NUCLEAR_DSL_FUSION_POOL_FUSION_HPP

#include <algorithm>
#include <stdexcept>

#include "../../threading/ReactionTask.hpp"
#include "../operation/DSLProxy.hpp"
#include "FindWords.hpp"
#include "has_nuclear_dsl_method.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Make a SFINAE type to check if a word has a pool method
        HAS_NUCLEAR_DSL_METHOD(pool);

        // Default case where there are no pool words
        template <typename Words>
        struct PoolFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct PoolFuser<std::tuple<Word>> {

            template <typename DSL>
            static std::shared_ptr<const util::ThreadPoolDescriptor> pool(threading::ReactionTask& task) {

                // Return our pool
                return Word::template pool<DSL>(task);
            }
        };

        // Case where there are 2 or more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct PoolFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static util::ThreadPoolDescriptor pool(const threading::ReactionTask& /*task*/) {
                throw std::invalid_argument("Can not be a member of more than one pool");
            }
        };

        template <typename Word1, typename... WordN>
        struct PoolFusion : PoolFuser<FindWords<has_pool, Word1, WordN...>> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_POOL_FUSION_HPP
