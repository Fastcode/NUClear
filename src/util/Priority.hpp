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

#ifndef NUCLEAR_UTIL_PRIORITY_HPP
#define NUCLEAR_UTIL_PRIORITY_HPP

#include <algorithm>
#include <cstdint>
#include <type_traits>

namespace NUClear {
namespace util {

    enum class Priority : uint8_t { LOWEST = 0, LOW = 1, NORMAL = 2, HIGH = 3, HIGHEST = 4 };
    inline Priority prev(Priority value) {
        value = static_cast<Priority>(static_cast<std::underlying_type_t<Priority>>(value) - 1);
        value = std::min(value, Priority::LOWEST);
        return value;
    }
    constexpr Priority MAX_PRIORITY = Priority::HIGHEST;

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_PRIORITY_HPP
