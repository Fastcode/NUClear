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

#ifndef NUCLEAR_UTIL_DEREFERENCER_HPP
#define NUCLEAR_UTIL_DEREFERENCER_HPP

namespace NUClear {
namespace util {

    template <typename T>
    struct is_dereferenceable {
    private:
        typedef std::true_type yes;
        typedef std::false_type no;

        template <typename U>
        static auto test(int) -> decltype(*std::declval<U>(), yes());
        template <typename>
        static no test(...);

    public:
        static constexpr bool value = std::is_same<decltype(test<T>(0)), yes>::value;
    };

    template <typename T, bool B = is_dereferenceable<T>::value>
    struct DereferencedType {};

    template <typename T>
    struct DereferencedType<T, true> {
        using type = decltype(*std::declval<T>());
    };

    template <typename T>
    struct Dereferencer {
    private:
        T& value;

    public:
        Dereferencer(T& value) : value(value) {}

        // We can return the type as normal
        operator T&() {
            return value;
        }

        // We can return the type this dereferences to if dereference operator
        template <typename U = T>
        operator typename DereferencedType<U>::type() {
            return *value;
        }

        // We also allow converting to anything that the type we are passing is convertable to
        template <typename U, typename = std::enable_if_t<std::is_convertible<T, U>::value>>
        operator U() {
            return value;
        }
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_DEREFERENCER_HPP
