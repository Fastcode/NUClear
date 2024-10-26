/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#ifndef NUCLEAR_DSL_FUSION_PRE_RUN_FUSION_HPP
#define NUCLEAR_DSL_FUSION_PRE_RUN_FUSION_HPP

#include "../../threading/ReactionTask.hpp"
#include "../operation/DSLProxy.hpp"
#include "FindWords.hpp"
#include "has_nuclear_dsl_method.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /// Make a SFINAE type to check if a word has a pre_run method
        HAS_NUCLEAR_DSL_METHOD(pre_run);

        // Default case where there are no pre_run words
        template <typename Words>
        struct PreRunFuser {};

        // Case where there is only a single word remaining
        template <typename Word>
        struct PreRunFuser<std::tuple<Word>> {

            template <typename DSL>
            static void pre_run(threading::ReactionTask& task) {

                // Run our remaining pre_run
                Word::template pre_run<DSL>(task);
            }
        };

        // Case where there is more 2 more more words remaining
        template <typename Word1, typename Word2, typename... WordN>
        struct PreRunFuser<std::tuple<Word1, Word2, WordN...>> {

            template <typename DSL>
            static void pre_run(threading::ReactionTask& task) {

                // Run our pre_run
                Word1::template pre_run<DSL>(task);

                // Run the rest of our pre_runs
                PreRunFuser<std::tuple<Word2, WordN...>>::template pre_run<DSL>(task);
            }
        };

        template <typename Word1, typename... WordN>
        struct PreRunFusion : PreRunFuser<FindWords<has_pre_run, Word1, WordN...>> {};

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_PRE_RUN_FUSION_HPP
