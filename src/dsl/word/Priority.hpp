/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_PRIORITY_HPP
#define NUCLEAR_DSL_WORD_PRIORITY_HPP

#include "../../threading/Reaction.hpp"
#include "../../util/Priority.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        /**
         * Task priority can be controlled using an assigned setting.
         *
         * @code on<Trigger<T, ...>, Priority<util::Priority::HIGH>>() @endcode
         * The PowerPlant uses this setting to determine the scheduling order in the threadpool, as well as assign a
         * priority to the thread on the OS.
         *
         * The available priority settings are:
         *
         * <b>HIGHEST:</b>
         * <b>HIGH:</b>
         * <b>NORMAL:</b>
         * <b>LOW:</b>
         * <b>LOWEST:</b>
         *
         * @par Default Behaviour
         *  @code on<Trigger<T>>() @endcode
         *  When the priority is not specified, tasks will be assigned a default setting; NORMAL.
         *
         * @attention
         *  If the OS allows the user to set thread priority, this word will also be used to assign the priority of the
         *  thread in its runtime environment.
         *
         * @par Implements
         *  Fusion
         */
        template <util::Priority value>
        struct Priority {
            template <typename DSL>
            static util::Priority priority(const threading::ReactionTask& /*task*/) {
                return value;
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_PRIORITY_HPP
