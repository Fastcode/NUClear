/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include "../../util/MetaProgramming.hpp"

namespace NUClear {
namespace dsl {
    namespace validation {

        /**
         * @brief Validates the provided DSL is able to be used by the system and give a more sensible error
         *
         * @details This is where checks to see if the DSL words that were provided make sense and are useable.
         *          For example, it will check to make sure there is at least one bind function there. If there is
         *          not a bind function, there is no way the provided callback will ever run.
         *
         * @tparam Words the DSL words to be validated
         */
        template <typename... Words>
        struct ValidateDSL {

            // Check that at least one word element has a bind component
            static_assert(Any<fusion::has_bind<Words>..., fusion::has_bind<operation::DSLProxy<Words>>...>::value,
                          "The provided DSL sentence does not have any components that bind a function");
        };

        // TODO Test that the function signature matches the arguments that will be obtained by get

        // TODO Test that the function signature uses the correct constness in the arguments

    }  // namespace validation
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_VALIDATION_VALIDATION_HPP
