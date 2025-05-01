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

#ifndef NUCLEAR_DSL_WORD_MAIN_THREAD_HPP
#define NUCLEAR_DSL_WORD_MAIN_THREAD_HPP

#include "Pool.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        namespace pool {
            /**
             * This struct is here to define the main thread pool.
             */
            struct Main {
                static constexpr const char* name = "Main";
                static constexpr int concurrency  = 1;
            };
        }  // namespace pool

        /**
         * This is used to specify that the associated task will need to execute using the main thread.
         *
         * @code on<Trigger<T, ...>, MainThread>() @endcode
         * This can be used with graphics related tasks.
         * For example, OpenGL requires all calls to be made from the main thread.
         */
        struct MainThread : Pool<pool::Main> {};

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_MAIN_THREAD_HPP
