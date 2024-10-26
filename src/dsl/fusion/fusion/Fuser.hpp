
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

#ifndef NUCLEAR_DSL_FUSION_FUSION_FUSER_HPP
#define NUCLEAR_DSL_FUSION_FUSION_FUSER_HPP

#include <algorithm>
#include <stdexcept>
#include <tuple>

#include "../../../threading/ReactionTask.hpp"
#include "../../../util/tuplify.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        // Default case where there are no words
        template <template <typename> class Hook, typename Words>
        struct Fuser {};

        // Case where there is only a single word remaining
        template <template <typename> class Hook, typename Word>
        struct Fuser<Hook, std::tuple<Word>> {

            template <typename DSL, typename... Args>
            static auto call(threading::ReactionTask& task, Args&&... args)
                -> decltype(util::tuplify(Caller<Hook>::template call<Word, DSL>(task, std::forward<Args>(args)...))) {
                return util::tuplify(Caller<Hook>::template call<Word, DSL>(task, std::forward<Args>(args)...));
            }
        };

        // Case where there are 2 or more words remaining
        template <template <typename> class Hook, typename Word1, typename... WordN>
        struct Fuser<Hook, std::tuple<Word1, WordN...>> {

            template <typename DSL, typename... Args>
            static auto call(const threading::ReactionTask& task, Args&&... args)
                -> decltype(util::tuplify(Hook<Word1>::template merge<DSL>(
                    Fuser<Hook, std::tuple<Word1>>::call(task, std::forward<Args>(args)...),
                    Fuser<Hook, std::tuple<WordN...>>::call(task, std::forward<Args>(args)...)))) {
                return util::tuplify(Hook<Word1>::template merge<DSL>(
                    Fuser<Hook, std::tuple<Word1>>::call(task, std::forward<Args>(args)...),
                    Fuser<Hook, std::tuple<WordN...>>::call(task, std::forward<Args>(args)...)));
            }
        };

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_FUSION_FUSER_HPP
