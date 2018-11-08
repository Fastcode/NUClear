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

#ifndef NUCLEAR_UTIL_METAPROGRAMMING_HPP
#define NUCLEAR_UTIL_METAPROGRAMMING_HPP

#include <type_traits>

/**
 * These classes are put in the global namespace rather than utility
 * This is so they can be used without any preamble (which would defeat the point of having them to shorten
 * metaprogramming)
 */
namespace NUClear {

/**
 * @brief Becomes true_type if all of the predicates pass, and false_type if any fail.
 *
 * @tparam T the set of condtions to check.
 *
 * @return true_type if all of the conditions evaluate to true, false_type otherwise.
 */
template <typename... T>
struct All : std::true_type {};
template <typename Head, typename... Tail>
struct All<Head, Tail...> : std::conditional_t<Head::value, All<Tail...>, std::false_type> {};

/**
 * @brief Becomes true_type if any of the predicates pass, and false_type they all fail.
 *
 * @tparam T the set of condtions to check.
 *
 * @return true_type if any of the conditions evaluate to true, false_type otherwise.
 */
template <typename... T>
struct Any : std::false_type {};
template <typename Head, typename... Tail>
struct Any<Head, Tail...> : std::conditional_t<Head::value, std::true_type, Any<Tail...>> {};

/**
 * @brief Becomes the inverse to the boolean type passed.
 *
 * @tparam Condition the type to attempt to inverse.
 *
 * @return true_type if T is false_type, false_type if T is true_type.
 */
template <typename Condition>
using Not = std::conditional_t<Condition::value, std::false_type, std::true_type>;

}  // namespace NUClear

#endif  // NUCLEAR_UTIL_METAPROGRAMMING_HPP
