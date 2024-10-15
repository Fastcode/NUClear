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
#ifndef NUCLEAR_DSL_FUSION_HOOK_RUN_INLINE_HPP
#define NUCLEAR_DSL_FUSION_HOOK_RUN_INLINE_HPP

namespace NUClear {
namespace dsl {
    namespace fusion {
        namespace hook {

            template <typename Word>
            struct RunInline {
            public:
                template <typename DSL, typename... Args>
                static auto call(Args&&... args)
                    -> decltype(Word::template run_inline<DSL>(std::forward<Args>(args)...)) {
                    return Word::template run_inline<DSL>(std::forward<Args>(args)...);
                }

                template <typename DSL>
                static util::Inline merge(const util::Inline& lhs, const util::Inline& rhs) {
                    // Must agree or make a choice
                    if ((lhs == util::Inline::ALWAYS && rhs == util::Inline::NEVER)
                        || (lhs == util::Inline::NEVER && rhs == util::Inline::ALWAYS)) {
                        throw std::logic_error("Cannot both always and never inline a reaction");
                    }

                    // Otherwise return the one that made lhs choice
                    return lhs == util::Inline::NEUTRAL ? rhs : lhs;
                }
            };

        }  // namespace hook
    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_HOOK_RUN_INLINE_HPP
