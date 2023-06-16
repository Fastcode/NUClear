/*
 * Copyright (C) 2023      Alex Biddulph <bidskii@gmail.com>
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

#ifndef NUCLEAR_DSL_WORD_GROUP_HPP
#define NUCLEAR_DSL_WORD_GROUP_HPP

#include <map>
#include <mutex>
#include <typeindex>

#include "../../threading/ReactionTask.hpp"
#include "../../util/GroupDescriptor.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        template <typename GroupType>
        struct Group {

            // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
            static std::map<std::type_index, uint64_t> group_id;
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
            static std::mutex mutex;

            template <typename DSL>
            static inline util::GroupDescriptor group(threading::Reaction& /*reaction*/) {

                const std::lock_guard<std::mutex> lock(mutex);
                if (group_id.count(typeid(GroupType)) == 0) {
                    group_id[typeid(GroupType)] = util::GroupDescriptor::get_new_group_id();
                }
                return util::GroupDescriptor{group_id[typeid(GroupType)], 1};
            }
        };

        // Initialise the static map and mutex
        template <typename GroupType>
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        std::map<std::type_index, uint64_t> Group<GroupType>::group_id;
        template <typename GroupType>
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        std::mutex Group<GroupType>::mutex;

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_GROUP_HPP
