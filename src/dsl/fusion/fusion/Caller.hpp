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

#ifndef NUCLEAR_DSL_FUSION_FUSION_CALLER_HPP
#define NUCLEAR_DSL_FUSION_FUSION_CALLER_HPP

#include <algorithm>
#include <stdexcept>

#include "../../../threading/ReactionTask.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /**
         * Wraps the word such that we can call one of a set of options based on what arguments they take.
         *
         * @tparam Hook the template type which wraps a specific hook
         */
        template <template <typename> class Hook>
        struct Caller {
            // Call giving a reference to the ReactionTask
            template <typename Word, typename DSL, typename... Args>
            static auto call(threading::ReactionTask& task, Args&&... args)
                -> decltype(Hook<Word>::template call<DSL>(task, std::forward<Args>(args)...)) {
                return Hook<Word>::template call<DSL>(task, std::forward<Args>(args)...);
            }

            // Call giving a reference to the Reaction
            template <typename Word, typename DSL, typename... Args>
            static auto call(threading::ReactionTask& task, Args&&... args)
                -> decltype(Hook<Word>::template call<DSL>(*task.parent, std::forward<Args>(args)...)) {
                return Hook<Word>::template call<DSL>(*task.parent, std::forward<Args>(args)...);
            }

            // Call giving a shared_ptr to the Reaction
            template <typename Word, typename DSL, typename... Args>
            static auto call(threading::ReactionTask& task, Args&&... args)
                -> decltype(Hook<Word>::template call<DSL>(task.parent, std::forward<Args>(args)...)) {
                return Hook<Word>::template call<DSL>(task.parent, std::forward<Args>(args)...);
            }

            // Call giving a reference to the Reactor
            template <typename Word, typename DSL, typename... Args>
            static auto call(threading::ReactionTask& task, Args&&... args)
                -> decltype(Hook<Word>::template call<DSL>(task.parent->reactor, std::forward<Args>(args)...)) {
                return Hook<Word>::template call<DSL>(task.parent->reactor, std::forward<Args>(args)...);
            }

            // Call giving no additional arguments
            template <typename Word, typename DSL, typename... Args>
            static auto call(threading::ReactionTask& /*task*/, Args&&... args)
                -> decltype(Hook<Word>::template call<DSL>(std::forward<Args>(args)...)) {
                return Hook<Word>::template call<DSL>(std::forward<Args>(args)...);
            }
        };

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_FUSION_CALLER_HPP
