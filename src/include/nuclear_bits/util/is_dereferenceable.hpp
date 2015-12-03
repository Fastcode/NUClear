/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_UTIL_DEREFERENCEABLE_H
#define NUCLEAR_UTIL_DEREFERENCEABLE_H

#include "MetaProgramming.hpp"

namespace NUClear {
    namespace util {

        template<typename T>
        struct is_dereferenceable {
        private:
            typedef std::true_type yes;
            typedef std::false_type no;

            template <typename U> static auto test(int) -> decltype(*std::declval<U>(), yes());
            template <typename> static no test(...);

        public:
            static constexpr bool value = std::is_same<decltype(test<T>(0)), yes>::value;
        };

        template <typename TData>
        auto dereference(TData&& d) -> EnableIf<is_dereferenceable<TData>, decltype(*d)> {
            return *d;
        }

        template <typename TData>
        auto dereference(TData&& d) -> EnableIf<Not<is_dereferenceable<TData>>, decltype(d)> {
            return d;
        }


        template <typename T>
        struct DereferenceTuple;

        template <typename... Ts>
        struct DereferenceTuple<std::tuple<Ts...>> {
            using type = std::tuple<decltype(dereference(std::declval<Ts>()))...>;
        };

    }
}

#endif
