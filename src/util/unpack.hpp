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

#ifndef NUCLEAR_UTIL_UNPACK_HPP
#define NUCLEAR_UTIL_UNPACK_HPP

namespace NUClear {
namespace util {

    /**
     * @brief Executes a series of function calls from a variadic template pack.
     *
     * @details
     *  This function is to be used as a helper to expand a variadic template pack into a series of function calls.
     *  As the variadic function pack can only be expanded in the situation where they are comma separated (and the
     *  comma is a syntactic separator not the comma operator) the only place this can expand is within a function call,
     *  or an initializer list (which can only accept a single type).
     *  This function serves that purpose by allowing a series of function calls to be expanded as its parameters. This
     *  will execute each of them without having to recursivly evaluate the pack. e.g.
     *      @code unpack(function(T)...); @endcode
     *
     *  In the case where the return type of the functions is void, then using the comma operator will allow the
     * functions
     *  to be run because it changes the return type of the expansion (function(T), 0) to int rather then void e.g.
     *      @code unpack((function(T), 0)...); @endcode
     *
     * @param t The resulting objects from executing the functions, these are ignored
     *
     * @tparam Ts the types of the resulting objects from executing the functions, these are ignored
     */
    template <typename... Ts>
    void unpack(Ts...) {}

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_UNPACK_HPP
