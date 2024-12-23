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

#ifndef NUCLEAR_DSL_FUSION_GROUP_FUSION_HPP
#define NUCLEAR_DSL_FUSION_GROUP_FUSION_HPP

#include <algorithm>
#include <set>
#include <stdexcept>

#include "../../threading/Reaction.hpp"
#include "../operation/DSLProxy.hpp"
#include "FindWords.hpp"
#include "has_nuclear_dsl_method.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Make a SFINAE type to check if a word has a group method
        HAS_NUCLEAR_DSL_METHOD(group);

        // Default case where there are no group words
        template <typename Words>
        struct GroupFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct GroupFuser<std::tuple<Word>> {

            template <typename DSL>
            static std::set<std::shared_ptr<const util::GroupDescriptor>> group(threading::ReactionTask& task) {

                // Return our group
                return Word::template group<DSL>(task);
            }
        };

        // Case where there are 2 or more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct GroupFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static std::set<std::shared_ptr<const util::GroupDescriptor>> group(threading::ReactionTask& task) {
                // Merge the list of groups together
                std::set<std::shared_ptr<const util::GroupDescriptor>> groups = Word1::template group<DSL>(task);
                auto remainder = GroupFuser<std::tuple<Word2, WordN...>>::template group<DSL>(task);
                groups.insert(remainder.begin(), remainder.end());

                return groups;
            }
        };

        template <typename Word1, typename... WordN>
        struct GroupFusion : GroupFuser<FindWords<has_group, Word1, WordN...>> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_GROUP_FUSION_HPP
