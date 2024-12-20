/*
 * MIT License
 *
 * Copyright (c) 2021 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_ONCE_HPP
#define NUCLEAR_DSL_WORD_ONCE_HPP

#include "../../threading/ReactionTask.hpp"
#include "Single.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * This is used to specify reactions which should occur only 1 time.
         *
         * @code on<Once>() @endcode
         * Any reactions listed with this DSL word will run only once.
         * This is the only time these reactions will run as the post_run Unbinds the current reaction.
         */
        struct Once : Single {

            // Post condition to unbind this reaction.
            template <typename DSL>
            static void post_run(threading::ReactionTask& task) {
                // Unbind:
                task.parent->unbind();
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_ONCE_HPP
