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

#ifndef NUCLEAR_DSL_OPERATION_DSLPROXY_HPP
#define NUCLEAR_DSL_OPERATION_DSLPROXY_HPP

namespace NUClear {
namespace dsl {
    namespace operation {

        /**
         * @brief A proxy template to be used to provide NUClear DSL methods without altering the original object
         *
         * @details
         *      Sometimes you want to make a type part of the DSL but cannot (or don't want to) alter the original
         *      object. In this case you are able to partially specialise this type and it will be used instead
         *      if the original does not have the functions needed.
         *
         * @tparam Word the type that will act as the DSL word
         */
        template <typename Word>
        struct DSLProxy {};

    }  // namespace operation
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_OPERATION_DSLPROXY_HPP
