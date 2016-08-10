/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_VALIDATION_VALIDATION_HPP
#define NUCLEAR_DSL_VALIDATION_VALIDATION_HPP

namespace NUClear {
    namespace dsl {
        namespace validation {

            template <typename... Word>
            struct ValidateDSL {

                // Check that at least one word element has a bind component
                static_assert(Any<fusion::has_bind<Word>..., fusion::has_bind<operation::DSLProxy<Word>>...>::value, "The provided DSL sentence does not have any components that bind a function");
            };

            // TODO Test that the function signature matches the arguments that will be obtained by get

            // TODO Test that the function signature uses the correct constness in the arguments

            // TODO Test that the function provides at least a bind (otherwise it will never run)

        }  // namespace validation
    }  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_VALIDATION_VALIDATION_HPP
