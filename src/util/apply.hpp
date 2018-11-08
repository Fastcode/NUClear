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

#ifndef NUCLEAR_UTIL_APPLY_HPP
#define NUCLEAR_UTIL_APPLY_HPP

#include <tuple>

#include "Dereferencer.hpp"
#include "RelevantArguments.hpp"
#include "Sequence.hpp"

namespace NUClear {
namespace util {

    /**
     * @brief Dereferences and uses the values from the tuple as the arguments for the function call.
     *
     * @details
     *  This function uses the values which are stored in the tuple and dereferences them as parameters in
     *  the callback function. It does this using the generated sequence of integers for this tuple. These
     *  values are then used to extract the function parameters in order.
     *
     * @param s the Sequence object which is passed in holding the int template pack
     *
     * @tparam S the integer pack giving the ordinal position of the tuple value to get
     */
    template <typename Function, int... S, typename... Arguments>
    void apply(Function&& function, const std::tuple<Arguments...>&& args, const Sequence<S...>&) {

        // Get each of the values from the tuple, dereference them and call the function with them
        // Also ensure that each value is a const reference
        function(Dereferencer<decltype(std::get<S>(args))>(std::get<S>(args))...);
    }

    template <typename Function, typename... Arguments>
    void apply_relevant(Function&& function, const std::tuple<Arguments...>&& args) {

        // Call apply with the relevant arguments
        apply(std::forward<Function>(function),
              std::move(args),
              typename RelevantArguments<Function, std::tuple<Dereferencer<Arguments>...>>::type());
    }

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_APPLY_HPP
