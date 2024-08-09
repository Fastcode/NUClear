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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>
#include <string>
#include <typeinfo>

struct TestSymbol {};

template <typename T>
struct TestTemplate {};

SCENARIO("Test the demangle function correctly demangles symbols", "[util][demangle]") {

    GIVEN("A valid mangled symbol") {
        const char* symbol         = typeid(int).name();
        const std::string expected = "int";

        WHEN("Demangle is called") {
            const std::string result = NUClear::util::demangle(symbol);

            THEN("It should return the demangled symbol") {
                REQUIRE(result == expected);
            }
        }
    }

    GIVEN("An empty symbol") {
        const char* symbol = "";
        const std::string expected;

        WHEN("Demangle is called") {
            const std::string result = NUClear::util::demangle(symbol);

            THEN("It should return an empty string") {
                REQUIRE(result == expected);
            }
        }
    }

    GIVEN("An invalid symbol") {
        const char* symbol         = "InvalidSymbol";
        const std::string expected = "InvalidSymbol";

        WHEN("Demangle is called") {
            const std::string result = NUClear::util::demangle(symbol);

            THEN("It should return the original symbol") {
                REQUIRE(result == expected);
            }
        }
    }

    GIVEN("A nullptr as the symbol") {
        const char* symbol = nullptr;

        WHEN("Demangle is called") {
            const std::string result = NUClear::util::demangle(symbol);

            THEN("It should return an empty string") {
                REQUIRE(result.empty());
            }
        }
    }

    GIVEN("A symbol from a struct") {
        const char* symbol         = typeid(TestSymbol).name();
        const std::string expected = "TestSymbol";

        WHEN("Demangle is called") {
            const std::string result = NUClear::util::demangle(symbol);

            THEN("It should return the demangled symbol") {
                REQUIRE(result == expected);
            }
        }
    }

    GIVEN("A symbol in a namespace") {
        const char* symbol         = typeid(NUClear::message::CommandLineArguments).name();
        const std::string expected = "NUClear::message::CommandLineArguments";

        WHEN("Demangle is called") {
            const std::string result = NUClear::util::demangle(symbol);

            THEN("It should return the demangled symbol") {
                REQUIRE(result == expected);
            }
        }
    }

    GIVEN("A templated symbol") {
        const char* symbol         = typeid(TestTemplate<int>).name();
        const std::string expected = "TestTemplate<int>";

        WHEN("Demangle is called") {
            const std::string result = NUClear::util::demangle(symbol);

            THEN("It should return the demangled symbol") {
                REQUIRE(result == expected);
            }
        }
    }
}
