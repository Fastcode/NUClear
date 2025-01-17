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

#ifndef NUCLEAR_UTIL_STRING_JOIN_HPP
#define NUCLEAR_UTIL_STRING_JOIN_HPP

#include <sstream>
#include <string>

namespace NUClear {
namespace util {

    namespace detail {
        // No argument base case
        inline void do_string_join(std::stringstream& /*out*/, const std::string& /*delimiter*/) {
            // Terminating case with no arguments, do nothing
        }

        // Single argument case
        template <typename Last>
        void do_string_join(std::stringstream& out, const std::string& /*delimiter*/, Last&& last) {
            out << std::forward<Last>(last);
        }

        // Two or more arguments case
        template <typename First, typename Second, typename... Remainder>
        void do_string_join(std::stringstream& out,
                            const std::string& delimiter,
                            First&& first,
                            Second&& second,
                            Remainder&&... remainder) {
            out << std::forward<First>(first) << delimiter;
            do_string_join(out, delimiter, std::forward<Second>(second), std::forward<Remainder>(remainder)...);
        }
    }  // namespace detail

    /**
     * Join a list of arguments into a single string with a delimiter between each argument.
     */
    template <typename... Arguments>
    std::string string_join(const std::string& delimiter, Arguments&&... args) {
        std::stringstream out;
        detail::do_string_join(out, delimiter, std::forward<Arguments>(args)...);
        return out.str();
    }

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_STRING_JOIN_HPP
