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

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "util/serialise/Serialise.hpp"

namespace {

/**
 * A type that is trivially copyable but carries its own wire format.
 *
 * A code generator's message class is the motivating case: it is a plain struct of scalars, so it is trivially
 * copyable, yet it has an encoding and a type identity of its own that a byte copy would bypass.
 */
struct Encoded {
    uint16_t value{0};
};

/// Marks the family of types that carry their own encoding, as a generator would for everything it emits
template <typename T>
struct is_encoded : std::is_same<T, Encoded> {};

/// The identity these types define for themselves, which is not their demangled C++ name
constexpr uint64_t ENCODED_HASH = 0x0123456789ABCDEFULL;

/// The wire format these types define for themselves, which is not their object representation
constexpr uint8_t ENCODED_PREFIX = 0xAB;

}  // namespace

namespace NUClear {
namespace util {
    namespace serialise {

        /**
         * Opt the family out of the byte copy.
         *
         * Without this the partial specialisation below is ambiguous with the trivially copyable one: neither is more
         * specialised than the other, so the two are equally good matches and the instantiation is ill-formed. A full
         * specialisation would win outright, but that is one specialisation per type rather than one for everything a
         * generator emits.
         */
        template <typename T>
        struct serialise_trivially<T, std::enable_if_t<is_encoded<T>::value>> : std::false_type {};

        template <typename T>
        struct Serialise<T, std::enable_if_t<is_encoded<T>::value, T>> {
            static std::vector<uint8_t> serialise(const T& in) {
                return {ENCODED_PREFIX, static_cast<uint8_t>(in.value >> 8), static_cast<uint8_t>(in.value & 0xFF)};
            }

            static T deserialise(const std::vector<uint8_t>& in) {
                if (in.size() != 3 || in[0] != ENCODED_PREFIX) {
                    throw std::length_error("Not an encoded value");
                }
                return T{static_cast<uint16_t>((static_cast<uint16_t>(in[1]) << 8) | in[2])};
            }

            static uint64_t hash() {
                return ENCODED_HASH;
            }
        };

    }  // namespace serialise
}  // namespace util
}  // namespace NUClear

SCENARIO("A trivially copyable type can provide its own serialisation", "[util][serialise][trivially]") {

    GIVEN("a trivially copyable type that has opted out of the byte copy") {
        STATIC_REQUIRE(std::is_trivially_copyable<Encoded>::value);
        STATIC_REQUIRE_FALSE(NUClear::util::serialise::serialise_trivially<Encoded>::value);

        const Encoded in{0x1234};

        WHEN("it is serialised") {
            const auto serialised = NUClear::util::serialise::Serialise<Encoded>::serialise(in);

            THEN("its own wire format is used rather than its object representation") {
                REQUIRE(serialised == std::vector<uint8_t>{ENCODED_PREFIX, 0x12, 0x34});
            }
        }

        WHEN("it is round tripped") {
            const auto serialised   = NUClear::util::serialise::Serialise<Encoded>::serialise(in);
            const auto deserialised = NUClear::util::serialise::Serialise<Encoded>::deserialise(serialised);

            THEN("the deserialised value is the same as the input") {
                REQUIRE(deserialised.value == in.value);
            }
        }

        WHEN("its hash is taken") {
            THEN("it is the identity the type defines rather than one derived from its C++ name") {
                REQUIRE(NUClear::util::serialise::Serialise<Encoded>::hash() == ENCODED_HASH);
            }
        }
    }

    GIVEN("a trivially copyable type that has not opted out") {
        STATIC_REQUIRE(NUClear::util::serialise::serialise_trivially<uint32_t>::value);

        const uint32_t in = 0xCAFEFECA;  // Mirrored so that endianess doesn't matter for the test

        WHEN("it is serialised") {
            const auto serialised = NUClear::util::serialise::Serialise<uint32_t>::serialise(in);

            THEN("it still uses the byte copy") {
                REQUIRE(serialised.size() == sizeof(uint32_t));
                REQUIRE(serialised == std::vector<uint8_t>{0xCA, 0xFE, 0xFE, 0xCA});
            }
        }
    }
}
