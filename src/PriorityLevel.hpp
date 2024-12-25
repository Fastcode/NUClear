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

#ifndef NUCLEAR_PRIORITY_LEVEL_HPP
#define NUCLEAR_PRIORITY_LEVEL_HPP

#include <cstdint>
#include <string>

namespace NUClear {

class PriorityLevel {
public:
    enum Value : uint8_t { IDLE = 0, LOW = 1, NORMAL = 2, HIGH = 3, REALTIME = 4 };

    /**
     * Construct a PriorityLevel from a Value
     *
     * @param value The value to construct the PriorityLevel from
     */
    constexpr PriorityLevel(const Value& value = Value::NORMAL) noexcept : value(value) {}

    /**
     * Construct a PriorityLevel from a string
     *
     * @param level The string to construct the PriorityLevel from
     */
    PriorityLevel(const std::string& level)
        : value(level == "IDLE"       ? Value::IDLE
                : level == "LOW"      ? Value::LOW
                : level == "NORMAL"   ? Value::NORMAL
                : level == "HIGH"     ? Value::HIGH
                : level == "REALTIME" ? Value::REALTIME
                                      : Value::NORMAL) {}

    /**
     * A call operator which will return the value of the PriorityLevel
     * This can be useful in situations where the implicit conversion operators are ambiguous.
     *
     * @return The value of the PriorityLevel
     */
    constexpr Value operator()() const {
        return value;
    }

    /**
     * A conversion operator which will return the value of the PriorityLevel
     *
     * @return The value of the PriorityLevel
     */
    constexpr operator Value() const {
        return value;
    }

    /**
     * A conversion operator which will return the string representation of the PriorityLevel
     *
     * @return The string representation of the PriorityLevel
     */
    operator std::string() const {
        return value == Value::IDLE       ? "IDLE"
               : value == Value::LOW      ? "LOW"
               : value == Value::NORMAL   ? "NORMAL"
               : value == Value::HIGH     ? "HIGH"
               : value == Value::REALTIME ? "REALTIME"
                                          : "UNKNOWN";
    }

    /**
     * Stream the PriorityLevel to an ostream, it will output the string representation of the PriorityLevel
     *
     * @param os The ostream to output to
     * @param level The PriorityLevel to output
     *
     * @return The ostream that was passed in
     */
    friend std::ostream& operator<<(std::ostream& os, const PriorityLevel& level) {
        return os << static_cast<std::string>(level);
    }

    // Operators to compare PriorityLevel values and PriorityLevel to Value
    // clang-format off
    friend constexpr bool operator<(const PriorityLevel& lhs, const PriorityLevel& rhs) { return lhs.value < rhs.value; }
    friend constexpr bool operator>(const PriorityLevel& lhs, const PriorityLevel& rhs) { return lhs.value > rhs.value; }
    friend constexpr bool operator<=(const PriorityLevel& lhs, const PriorityLevel& rhs) { return lhs.value <= rhs.value; }
    friend constexpr bool operator>=(const PriorityLevel& lhs, const PriorityLevel& rhs) { return lhs.value >= rhs.value; }
    friend constexpr bool operator==(const PriorityLevel& lhs, const PriorityLevel& rhs) { return lhs.value == rhs.value; }
    friend constexpr bool operator!=(const PriorityLevel& lhs, const PriorityLevel& rhs) { return lhs.value != rhs.value; }

    friend constexpr bool operator<(const PriorityLevel& lhs, const Value& rhs) { return lhs.value < rhs; }
    friend constexpr bool operator>(const PriorityLevel& lhs, const Value& rhs) { return lhs.value > rhs; }
    friend constexpr bool operator<=(const PriorityLevel& lhs, const Value& rhs) { return lhs.value <= rhs; }
    friend constexpr bool operator>=(const PriorityLevel& lhs, const Value& rhs) { return lhs.value >= rhs; }
    friend constexpr bool operator==(const PriorityLevel& lhs, const Value& rhs) { return lhs.value == rhs; }
    friend constexpr bool operator!=(const PriorityLevel& lhs, const Value& rhs) { return lhs.value != rhs; }
    friend constexpr bool operator<(const Value& lhs, const PriorityLevel& rhs) { return lhs < rhs.value; }
    friend constexpr bool operator>(const Value& lhs, const PriorityLevel& rhs) { return lhs > rhs.value; }
    friend constexpr bool operator<=(const Value& lhs, const PriorityLevel& rhs) { return lhs <= rhs.value; }
    friend constexpr bool operator>=(const Value& lhs, const PriorityLevel& rhs) { return lhs >= rhs.value; }
    friend constexpr bool operator==(const Value& lhs, const PriorityLevel& rhs) { return lhs == rhs.value; }
    friend constexpr bool operator!=(const Value& lhs, const PriorityLevel& rhs) { return lhs != rhs.value; }

    friend  bool operator<(const PriorityLevel& lhs, const std::string& rhs) { return static_cast<std::string>(lhs) < rhs; }
    friend  bool operator>(const PriorityLevel& lhs, const std::string& rhs) { return static_cast<std::string>(lhs) > rhs; }
    friend  bool operator<=(const PriorityLevel& lhs, const std::string& rhs) { return static_cast<std::string>(lhs) <= rhs; }
    friend  bool operator>=(const PriorityLevel& lhs, const std::string& rhs) { return static_cast<std::string>(lhs) >= rhs; }
    friend  bool operator==(const PriorityLevel& lhs, const std::string& rhs) { return static_cast<std::string>(lhs) == rhs; }
    friend  bool operator!=(const PriorityLevel& lhs, const std::string& rhs) { return static_cast<std::string>(lhs) != rhs; }
    friend  bool operator<(const std::string& lhs, const PriorityLevel& rhs) { return lhs < static_cast<std::string>(rhs); }
    friend  bool operator>(const std::string& lhs, const PriorityLevel& rhs) { return lhs > static_cast<std::string>(rhs); }
    friend  bool operator<=(const std::string& lhs, const PriorityLevel& rhs) { return lhs <= static_cast<std::string>(rhs); }
    friend  bool operator>=(const std::string& lhs, const PriorityLevel& rhs) { return lhs >= static_cast<std::string>(rhs); }
    friend  bool operator==(const std::string& lhs, const PriorityLevel& rhs) { return lhs == static_cast<std::string>(rhs); }
    friend  bool operator!=(const std::string& lhs, const PriorityLevel& rhs) { return lhs != static_cast<std::string>(rhs); }
    // clang-format on

private:
    /// The stored enum value
    Value value;
};

}  // namespace NUClear

#endif  // NUCLEAR_PRIORITY_LEVEL_HPP
