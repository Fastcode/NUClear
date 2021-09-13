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

#ifndef NUCLEAR_UTIL_SEQUENCE_HPP
#define NUCLEAR_UTIL_SEQUENCE_HPP

namespace NUClear {
namespace util {

    /**
     * @brief This class is used to hold a sequence of integers as a variadic pack
     *
     * @tparam S the variadic pack containing the sequence
     */
    template <int... S>
    struct Sequence {
        static constexpr int length = sizeof...(S);
    };

    // Anonymous
    namespace {

        /**
         * @brief Generate a sequence of numbers between a start and an end, this is the entry case
         */
        template <int Start, int End, typename Seq = Sequence<>>
        struct GenSequence;

        /**
         * Generates sequence
         */
        template <int Start, int End, int... Seq>
        struct GenSequence<Start, End, Sequence<Seq...>>
            : std::conditional_t<(Start > End),
                                 GenSequence<0,
                                             0>  // If we have been given an invalid sequence, just return an empty one
                                 ,
                                 GenSequence<Start + 1, End, Sequence<Seq..., Start>>> {};

        /**
         * Runs when start and end are the same, terminates
         */
        template <int StartAndEnd, int... Seq>
        struct GenSequence<StartAndEnd, StartAndEnd, Sequence<Seq...>> {
            using type = Sequence<Seq...>;
        };
    }  // namespace

    /**
     * @brief Holds a generated integer sequence of numbers as a variadic pack.
     *
     * @details
     *  This class is used to generate a variadic template pack which contains all of the integers from Start to End
     * (non inclusive).
     *  This can then be used in other templates or tempalate expansions as needed (for instance it is useful for
     *  expanding tuples).
     *
     * @tparam Start    the number to start counting from
     * @tparam End      the number to finish counting at (non inclusive)
     */
    template <int Start, int End>
    using GenerateSequence = typename GenSequence<Start, End>::type;

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_SEQUENCE_HPP
