/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#ifndef NUCLEAR_DSL_FUSION_HAS_RUN_INLINE_HPP
#define NUCLEAR_DSL_FUSION_HAS_RUN_INLINE_HPP

#include "../../threading/ReactionTask.hpp"
#include "NoOp.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /**
         * SFINAE struct to test if the passed class has a run_inline function that conforms to the NUClear DSL.
         *
         * @tparam T the class to check
         */
        template <typename T>
        struct has_run_inline {
        private:
            using yes = std::true_type;
            using no  = std::false_type;

            template <typename U>
            static auto test(int)
                -> decltype(U::template run_inline<ParsedNoOp>(std::declval<threading::ReactionTask&>()), yes());
            template <typename>
            static no test(...);

        public:
            static constexpr bool value = std::is_same<decltype(test<T>(0)), yes>::value;
        };

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_HAS_RUN_INLINE_HPP
