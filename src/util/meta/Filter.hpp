/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_META_FILTER_HPP
#define NUCLEAR_UTIL_META_FILTER_HPP

#include <tuple>
#include <type_traits>

namespace NUClear {

namespace util {
    namespace meta {

        template <template <typename> class Check, typename Current, typename Found = std::tuple<>>
        struct DoFilter;

        /**
         * Metafunction that extracts all of the types which pass the check from the tuple
         *
         * @tparam Check   The check to perform on each of the types
         * @tparam T       The current type being inspected
         * @tparam Ts      The remainder of the types to check
         * @tparam Found   The list of types which have already passed the check
         *
         * @return A tuple of the types which passed the check
         */
        template <template <typename> class Check, typename T1, typename... Ts, typename... Found>
        struct DoFilter<Check, std::tuple<T1, Ts...>, std::tuple<Found...>>
            : std::conditional_t<Check<T1>::value,
                                 DoFilter<Check, std::tuple<Ts...>, std::tuple<Found..., T1>>,
                                 DoFilter<Check, std::tuple<Ts...>, std::tuple<Found...>>> {};

        /**
         * Termination case for the Filter metafunction.
         *
         * @tparam Found The words that passed the filter check
         */
        template <template <typename> class Check, typename... Found>
        struct DoFilter<Check, std::tuple<>, std::tuple<Found...>> {
            using type = std::tuple<Found...>;
        };

    }  // namespace meta
}  // namespace util

/**
 * Metafunction that extracts all of the types which pass the check from the tuple
 *
 * @tparam Check The check to perform on each of the types
 * @tparam Ts    The types to check
 *
 * @return A tuple of the types which passed the check
 */
template <template <typename> class Check, typename... Ts>
using Filter = typename util::meta::DoFilter<Check, std::tuple<Ts...>>::type;

}  // namespace NUClear

#endif  // NUCLEAR_UTIL_META_FILTER_HPP
