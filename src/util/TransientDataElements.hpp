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

#ifndef NUCLEAR_UTIL_TRANSIENTDATAELEMENTS_HPP
#define NUCLEAR_UTIL_TRANSIENTDATAELEMENTS_HPP

namespace NUClear {
namespace util {

    template <typename Input, int Index = 0, typename Output = std::tuple<>, typename Indicies = Sequence<>>
    struct ExtractTransient;

    template <typename First, typename... Input, int Index, typename... Output, int... Indicies>
    struct ExtractTransient<std::tuple<First, Input...>, Index, std::tuple<Output...>, Sequence<Indicies...>>
        : std::conditional_t<
              dsl::trait::is_transient<First>::value,
              /*T*/
              ExtractTransient<std::tuple<Input...>,
                               Index + 1,
                               std::tuple<Output..., First>,
                               Sequence<Indicies..., Index>>,
              /*F*/ ExtractTransient<std::tuple<Input...>, Index + 1, std::tuple<Output...>, Sequence<Indicies...>>> {};

    template <int Index, typename... Output, int... Indicies>
    struct ExtractTransient<std::tuple<>, Index, std::tuple<Output...>, Sequence<Indicies...>> {
        using type                        = std::tuple<Output...>;
        using index                       = Sequence<Indicies...>;
        static constexpr const bool value = sizeof...(Output) > 0;
    };

    template <typename DSL>
    struct TransientDataElements : public ExtractTransient<decltype(DSL::get(std::declval<threading::Reaction&>()))> {};

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_TRANSIENTDATAELEMENTS_HPP
