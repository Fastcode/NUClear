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

#ifndef NUCLEAR_UTIL_TUPLIFY_HPP
#define NUCLEAR_UTIL_TUPLIFY_HPP

#include <tuple>

namespace NUClear {
namespace util {

    template <typename T>
    static inline std::tuple<T> tuplify(T&& data) {
        return std::make_tuple(std::forward<T>(data));
    }

    template <typename... Ts>
    static inline std::tuple<Ts...> tuplify(std::tuple<Ts...>&& tuple) {
        return std::move(tuple);
    }

    template <typename First, typename Second, typename... Remainder>
    static inline std::tuple<First, Second, Remainder...> detuplify(std::tuple<First, Second, Remainder...>&& tuple) {
        return std::move(tuple);
    }

    template <typename T>
    // Reaching here there is only one element in the tuple and it is moved
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    static inline T detuplify(std::tuple<T>&& tuple) {
        return std::move(std::get<0>(tuple));
    }

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_TUPLIFY_HPP
