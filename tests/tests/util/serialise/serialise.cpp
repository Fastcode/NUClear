/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#include "util/serialise/Serialise.hpp"

#include <array>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <iterator>
#include <list>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

SCENARIO("Serialisation works correctly on single primitives", "[util][serialise][single][primitive]") {

    GIVEN("a primitive value") {
        const uint32_t in = 0xCAFEFECA;  // Mirrored so that endianess doesn't matter for the test

        WHEN("it is serialised") {
            const auto serialised = NUClear::util::serialise::Serialise<uint32_t>::serialise(in);

            THEN("The serialised data is as expected") {
                REQUIRE(serialised.size() == sizeof(uint32_t));
                REQUIRE(serialised == std::vector<uint8_t>{0xCA, 0xFE, 0xFE, 0xCA});
            }
        }

        WHEN("it is round tripped through the serialise and deserialise functions") {
            const auto serialised   = NUClear::util::serialise::Serialise<uint32_t>::serialise(in);
            const auto deserialised = NUClear::util::serialise::Serialise<uint32_t>::deserialise(serialised);

            THEN("The deserialised data is the same as the input") {
                REQUIRE(deserialised == in);
            }
        }
    }

    GIVEN("serialised data for a primitive value") {
        const std::vector<uint8_t> in = {0xCA, 0xFE, 0xFE, 0xCA};

        WHEN("it is deserialised") {
            const auto deserialised = NUClear::util::serialise::Serialise<uint32_t>::deserialise(in);

            THEN("The deserialised data is as expected") {
                REQUIRE(deserialised == 0xCAFEFECA);
            }
        }

        WHEN("it is round tripped through the deserialise and serialise functions") {
            const auto deserialised = NUClear::util::serialise::Serialise<uint32_t>::deserialise(in);
            const auto serialised   = NUClear::util::serialise::Serialise<uint32_t>::serialise(deserialised);

            THEN("The serialised data is the same as the input") {
                REQUIRE(serialised == in);
            }
        }
    }

    GIVEN("serialised data that is too small") {
        const std::vector<uint8_t> in = {0xBA, 0xAD, 0xBA};

        WHEN("it is deserialised") {
            THEN("The deserialise function throws an exception") {
                REQUIRE_THROWS_AS(NUClear::util::serialise::Serialise<uint32_t>::deserialise(in), std::length_error);
            }
        }
    }

    GIVEN("serialised data that is too large") {
        const std::vector<uint8_t> in = {0xBA, 0xDB, 0xAD, 0xBA, 0xDB};

        WHEN("it is deserialised") {
            THEN("The deserialise function throws an exception") {
                REQUIRE_THROWS_AS(NUClear::util::serialise::Serialise<uint32_t>::deserialise(in), std::length_error);
            }
        }
    }
}


TEMPLATE_TEST_CASE("Scenario: Serialisation works correctly on iterables of primitives",
                   "[util][serialise][multiple][primitive]",
                   std::vector<uint32_t>,
                   std::list<uint32_t>) {

    GIVEN("a vector of primitive values") {
        const TestType in = {0xABBABAAB, 0xDEADADDE, 0xCAFEFECA, 0xBEEFEFBE};

        WHEN("it is serialised") {
            const auto serialised = NUClear::util::serialise::Serialise<TestType>::serialise(in);

            THEN("The serialised data is as expected") {
                const std::vector<uint8_t> expected =
                    {0xAB, 0xBA, 0xBA, 0xAB, 0xDE, 0xAD, 0xAD, 0xDE, 0xCA, 0xFE, 0xFE, 0xCA, 0xBE, 0xEF, 0xEF, 0xBE};
                REQUIRE(serialised == expected);
            }
        }

        WHEN("it is round tripped through the serialise and deserialise functions") {
            const auto serialised   = NUClear::util::serialise::Serialise<TestType>::serialise(in);
            const auto deserialised = NUClear::util::serialise::Serialise<TestType>::deserialise(serialised);

            THEN("The deserialised data is the same as the input") {
                REQUIRE(deserialised == in);
            }
        }
    }

    GIVEN("serialised data for multiple primitives") {
        const std::vector<uint8_t> in =
            {0xBE, 0xEF, 0xEF, 0xBE, 0xAB, 0xBA, 0xBA, 0xAB, 0xDE, 0xAD, 0xAD, 0xDE, 0xCA, 0xFE, 0xFE, 0xCA};

        WHEN("it is deserialised") {
            const auto deserialised = NUClear::util::serialise::Serialise<TestType>::deserialise(in);

            THEN("The deserialised data is as expected") {
                REQUIRE(deserialised.size() == 4);
                REQUIRE(*std::next(deserialised.begin(), 0) == 0xBEEFEFBE);
                REQUIRE(*std::next(deserialised.begin(), 1) == 0xABBABAAB);
                REQUIRE(*std::next(deserialised.begin(), 2) == 0xDEADADDE);
                REQUIRE(*std::next(deserialised.begin(), 3) == 0xCAFEFECA);
            }
        }

        WHEN("it is round tripped through the deserialise and serialise functions") {
            const auto deserialised = NUClear::util::serialise::Serialise<TestType>::deserialise(in);
            const auto serialised   = NUClear::util::serialise::Serialise<TestType>::serialise(deserialised);

            THEN("The serialised data is the same as the input") {
                REQUIRE(serialised == in);
            }
        }
    }

    GIVEN("serialised data that does not divide evenly into the size") {
        const std::vector<uint8_t> in = {0xBA, 0xAD, 0xBA, 0xBA, 0xAD, 0xBA};

        WHEN("it is deserialised") {
            THEN("The deserialise function throws an exception") {
                REQUIRE_THROWS_AS(NUClear::util::serialise::Serialise<uint32_t>::deserialise(in), std::length_error);
            }
        }
    }

    GIVEN("empty serialised data") {
        const std::vector<uint8_t> in;

        WHEN("it is deserialised") {
            const auto deserialised = NUClear::util::serialise::Serialise<TestType>::deserialise(in);

            THEN("The deserialised data is empty") {
                REQUIRE(deserialised.empty());
            }
        }
    }
}

struct TriviallyCopyable {
    uint8_t a;
    int8_t b;
    std::array<uint8_t, 2> c;

    friend bool operator==(const TriviallyCopyable& lhs, const TriviallyCopyable& rhs) {
        return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c[0] == rhs.c[0] && lhs.c[1] == rhs.c[1];
    }
};
static_assert(std::is_trivially_copyable<TriviallyCopyable>::value, "This type should be trivially copyable");

SCENARIO("Serialisation works correctly on single trivially copyable types", "[util][serialise][single][trivial]") {

    GIVEN("a trivially copyable value") {
        const TriviallyCopyable in = {0xFF, -1, {0xDE, 0xAD}};

        WHEN("it is serialised") {
            const auto serialised = NUClear::util::serialise::Serialise<TriviallyCopyable>::serialise(in);

            THEN("The serialised data is as expected") {
                REQUIRE(serialised.size() == sizeof(TriviallyCopyable));
                REQUIRE(serialised == std::vector<uint8_t>({0xFF, 0xFF, 0xDE, 0xAD}));
            }
        }

        WHEN("it is round tripped through the serialise and deserialise functions") {
            const auto serialised   = NUClear::util::serialise::Serialise<TriviallyCopyable>::serialise(in);
            const auto deserialised = NUClear::util::serialise::Serialise<TriviallyCopyable>::deserialise(serialised);

            THEN("The deserialised data is the same as the input") {
                REQUIRE(deserialised.a == in.a);
                REQUIRE(deserialised.b == in.b);
                REQUIRE(deserialised.c[0] == in.c[0]);
                REQUIRE(deserialised.c[1] == in.c[1]);
            }
        }
    }

    GIVEN("serialised data for a primitive value") {
        const std::vector<uint8_t> in = {0xCA, 0xFE, 0xFE, 0xCA};

        WHEN("it is deserialised") {
            const auto deserialised = NUClear::util::serialise::Serialise<TriviallyCopyable>::deserialise(in);

            THEN("The deserialised data is as expected") {
                REQUIRE(deserialised.a == 0xCA);
                REQUIRE(deserialised.b == -0x02);
                REQUIRE(deserialised.c[0] == 0xFE);
                REQUIRE(deserialised.c[1] == 0xCA);
            }
        }

        WHEN("it is round tripped through the deserialise and serialise functions") {
            const auto deserialised = NUClear::util::serialise::Serialise<TriviallyCopyable>::deserialise(in);
            const auto serialised   = NUClear::util::serialise::Serialise<TriviallyCopyable>::serialise(deserialised);

            THEN("The serialised data is the same as the input") {
                REQUIRE(serialised == in);
            }
        }
    }

    GIVEN("serialised data that is too small") {
        const std::vector<uint8_t> in = {0xCA, 0xFE, 0xFE};

        WHEN("it is deserialised") {
            THEN("The deserialise function throws an exception") {
                REQUIRE_THROWS_AS(NUClear::util::serialise::Serialise<TriviallyCopyable>::deserialise(in),
                                  std::length_error);
            }
        }
    }

    GIVEN("serialised data that is too large") {
        const std::vector<uint8_t> in = {0xCA, 0xFE, 0xFE, 0xCA, 0xFE, 0xFE};

        WHEN("it is deserialised") {
            THEN("The deserialise function throws an exception") {
                REQUIRE_THROWS_AS(NUClear::util::serialise::Serialise<TriviallyCopyable>::deserialise(in),
                                  std::length_error);
            }
        }
    }
}


TEMPLATE_TEST_CASE("Scenario: Serialisation works correctly on iterables of trivially copyable types",
                   "[util][serialise][multiple][trivial]",
                   std::vector<TriviallyCopyable>,
                   std::list<TriviallyCopyable>) {

    GIVEN("a vector of trivial values") {
        const TestType in = {{'h', 'e', {'l', 'o'}}, {'w', 'o', {'r', 'd'}}};

        WHEN("it is serialised") {
            const auto serialised = NUClear::util::serialise::Serialise<TestType>::serialise(in);

            THEN("The serialised data is as expected") {
                auto s = std::string(serialised.begin(), serialised.end());
                REQUIRE(serialised.size() == sizeof(uint32_t) * in.size());
                REQUIRE(s == "heloword");
            }
        }

        WHEN("it is round tripped through the serialise and deserialise functions") {
            const auto serialised   = NUClear::util::serialise::Serialise<TestType>::serialise(in);
            const auto deserialised = NUClear::util::serialise::Serialise<TestType>::deserialise(serialised);

            THEN("The deserialised data is the same as the input") {
                REQUIRE(deserialised == in);
            }
        }
    }

    GIVEN("serialised data for multiple trivials") {
        const std::string in_s = "Hello World!";
        const std::vector<uint8_t> in(in_s.begin(), in_s.end());

        WHEN("it is deserialised") {
            const auto deserialised = NUClear::util::serialise::Serialise<TestType>::deserialise(in);

            THEN("The deserialised data is as expected") {
                REQUIRE(deserialised.size() == 3);
                REQUIRE(*std::next(deserialised.begin(), 0) == TriviallyCopyable{'H', 'e', {'l', 'l'}});
                REQUIRE(*std::next(deserialised.begin(), 1) == TriviallyCopyable{'o', ' ', {'W', 'o'}});
                REQUIRE(*std::next(deserialised.begin(), 2) == TriviallyCopyable{'r', 'l', {'d', '!'}});
            }
        }

        WHEN("it is round tripped through the deserialise and serialise functions") {
            const auto deserialised = NUClear::util::serialise::Serialise<TestType>::deserialise(in);
            const auto serialised   = NUClear::util::serialise::Serialise<TestType>::serialise(deserialised);

            THEN("The serialised data is the same as the input") {
                REQUIRE(serialised == in);
            }
        }
    }

    GIVEN("serialised data that does not divide evenly into the size") {
        const std::vector<uint8_t> in = {0xBA, 0xAD, 0xBA, 0xBA, 0xAD, 0xBA, 0xBA, 0xAD, 0xBA};

        WHEN("it is deserialised") {
            THEN("The deserialise function throws an exception") {
                REQUIRE_THROWS_AS(NUClear::util::serialise::Serialise<uint32_t>::deserialise(in), std::length_error);
            }
        }
    }

    GIVEN("empty serialised data") {
        const std::vector<uint8_t> in;

        WHEN("it is deserialised") {
            const auto deserialised = NUClear::util::serialise::Serialise<TestType>::deserialise(in);

            THEN("The deserialised data is empty") {
                REQUIRE(deserialised.empty());
            }
        }
    }
}
