/*
 * MIT License
 *
 * Copyright (c) 2016 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_INLINE_HPP
#define NUCLEAR_DSL_WORD_INLINE_HPP

#include "../../threading/ReactionTask.hpp"
#include "../../util/Inline.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        struct Inline {

            /**
             * This word is used to specify that a reaction should be executed inline even if not emitted inline.
             *
             * @code on<Trigger<T, ...>, Inline::ALWAYS>>() @endcode
             * When this keyword is used, the reaction will always be inlined if it is not emitted using an inline emit.
             *
             * @par Implements
             *  Inline
             */
            struct ALWAYS {
                template <typename DSL>
                static util::Inline run_inline(const threading::ReactionTask& /*task*/) {
                    return util::Inline::ALWAYS;
                }
            };

            /**
             * This word is used to specify that a reaction should not be inlined.
             *
             * @code on<Trigger<T, ...>, Inline::NEVER>>() @endcode
             * When this keyword is used, the reaction will not be inlined if it is emitted using an inline emit.
             *
             * @par Implements
             *  Inline
             */
            struct NEVER {
                template <typename DSL>
                static util::Inline run_inline(const threading::ReactionTask& /*task*/) {
                    return util::Inline::NEVER;
                }
            };
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_INLINE_HPP
