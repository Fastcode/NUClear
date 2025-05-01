/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#include "util/string_join.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <iosfwd>
#include <string>
#include <typeinfo>

struct TestSymbol {
    friend std::ostream& operator<<(std::ostream& os, const TestSymbol& /*symbol*/) {
        return os << typeid(TestSymbol).name();
    }
};


SCENARIO("Test string_join correctly joins strings", "[util][string_join]") {

    GIVEN("An empty set of arguments") {
        const std::string delimiter = GENERATE("", ",", " ");

        WHEN("string_join is called") {
            const std::string result = NUClear::util::string_join(delimiter);

            THEN("It should return an empty string") {
                REQUIRE(result.empty());
            }
        }
    }

    GIVEN("A single string argument") {
        const std::string delimiter = GENERATE("", ",", " ");
        const std::string arg       = GENERATE("test", "string", "join");

        WHEN("string_join is called") {
            const std::string result = NUClear::util::string_join(delimiter, arg);

            THEN("It should return the argument") {
                REQUIRE(result == arg);
            }
        }
    }

    GIVEN("Two string arguments") {
        const std::string delimiter = GENERATE("", ",", " ");
        const std::string arg1      = GENERATE("test", "string", "join");
        const std::string arg2      = GENERATE("test", "string", "join");

        WHEN("string_join is called") {
            const std::string result = NUClear::util::string_join(delimiter, arg1, arg2);

            THEN("It should return the two arguments joined by the delimiter") {
                REQUIRE(result == arg1 + delimiter + arg2);
            }
        }
    }

    GIVEN("Three string arguments") {
        const std::string delimiter = GENERATE("", ",", " ");
        const std::string arg1      = GENERATE("test", "string", "join");
        const std::string arg2      = GENERATE("test", "string", "join");
        const std::string arg3      = GENERATE("test", "string", "join");

        WHEN("string_join is called") {
            const std::string result = NUClear::util::string_join(delimiter, arg1, arg2, arg3);

            THEN("It should return the three arguments joined by the delimiter") {
                REQUIRE(result == arg1 + delimiter + arg2 + delimiter + arg3);
            }
        }
    }

    GIVEN("A mix of string and non-string arguments") {
        const std::string delimiter = GENERATE("", ",", " ");
        const std::string arg1      = GENERATE("test", "string", "join");
        const std::string arg2      = GENERATE("test", "string", "join");
        const int arg3              = GENERATE(1, 2, 3);

        WHEN("string_join is called") {
            const std::string result = NUClear::util::string_join(delimiter, arg1, arg2, arg3);

            THEN("It should return the arguments joined by the delimiter") {
                REQUIRE(result == arg1 + delimiter + arg2 + delimiter + std::to_string(arg3));
            }
        }
    }

    GIVEN("A class with an overloaded operator<<") {
        const std::string delimiter = GENERATE("", ",", " ");
        const TestSymbol arg1;
        const TestSymbol arg2;

        WHEN("string_join is called") {
            const std::string result = NUClear::util::string_join(delimiter, arg1, arg2);

            THEN("It should return the arguments joined by the delimiter") {
                REQUIRE(result == typeid(TestSymbol).name() + delimiter + typeid(TestSymbol).name());
            }
        }
    }
}
