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

#ifndef NUCLEAR_DSL_OPERATION_UNBIND_HPP
#define NUCLEAR_DSL_OPERATION_UNBIND_HPP

#include "../../id.hpp"

namespace NUClear {
namespace dsl {
    namespace operation {

        /**
         * Signals that the reaction with this id servicing this type should be unbound and its resources cleaned up.
         *
         * When a reaction is finished and won't be run again, this type should be emitted along with the original DSL
         * word that created it.
         * This signals to its handler to clean it up and not run it again.
         *
         * @tparam Word The DSL word that created this binding (or another helper type)
         */
        template <typename Word>
        struct Unbind {
            explicit Unbind(const NUClear::id_t& id) : id(id) {};

            /// The id of the task to unbind
            const NUClear::id_t id{0};
        };

    }  // namespace operation
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_OPERATION_UNBIND_HPP
