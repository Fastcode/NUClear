/*
 * MIT License
 *
 * Copyright (c) 2022 NUClear Contributors
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
#include <vector>

namespace {  // Internal linkage

void do_nothing(std::vector<int>&& v) {
    // Do nothing with v
}

void assign(std::vector<int>&& v) {
    std::vector<int> v2 = v;
}

void assign_with_move(std::vector<int>&& v) {
    std::vector<int> v2 = std::move(v);
}

template <typename T>
void perfect_forwarding(T&& v) {
    std::vector<int> v2 = std::forward<T>(v);
}

}  // namespace

SCENARIO("Testing how rvalues are handled") {
    GIVEN("a vector with some data") {
        std::vector<int> v1 = {0, 1};

        WHEN("moving it to a function that does nothing") {
            do_nothing(std::move(v1));
            THEN("the vector still has the data") {
                CHECK(v1 == std::vector<int>{0, 1});
            }
        }

        WHEN("moving it to a function as an rvalue and assigns it to a new vector") {
            assign(std::move(v1));
            THEN("the vector still has the data") {
                REQUIRE(v1 == std::vector<int>{0, 1});
            }
        }

        WHEN("moving it to a function that assigns with perfect forwarding") {
            perfect_forwarding(std::move(v1));
            THEN("the vector is empty") {
                REQUIRE(v1 == std::vector<int>{});
            }
        }

        WHEN("moving it to a function as an rvalue and assigns it to a new vector with std::move") {
            assign_with_move(std::move(v1));
            THEN("the vector is empty") {
                REQUIRE(v1 == std::vector<int>{});
            }
        }

        WHEN("not moving it to a function with perfect forwarding") {
            perfect_forwarding(v1);
            THEN("the vector still has the data") {
                REQUIRE(v1 == std::vector<int>{0, 1});
            }
        }
    }
}
