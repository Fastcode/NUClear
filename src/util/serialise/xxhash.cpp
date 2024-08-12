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

#include "xxhash.hpp"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace NUClear {
namespace util {
    namespace serialise {

        namespace {

            /**
             * Returns a boolean value indicating whether the system is little-endian or not.
             *
             * This function creates a union of a 16-bit unsigned integer and an 8-bit unsigned integer.
             * It initializes the 16-bit integer to 1 and then checks whether the value of the 8-bit integer is
             * non-zero. If the 8-bit integer is non-zero, then the system is little-endian, otherwise it is big-endian.
             *
             * @return true if the system is little-endian, false if it is big-endian.
             */
            inline bool little_endian() {
                const union {  // NOSONAR
                    uint16_t a;
                    uint8_t b;
                } u{1};
                return u.b != 0;  // NOSONAR
            }

            /**
             * Reads a 32-bit integer from a character array in either little-endian or big-endian byte order.
             *
             * @param v The character array to read from.
             *
             * @return The 32-bit integer read from the character array.
             */
            uint32_t read32(const char* v) {
                return little_endian() ? (uint32_t(uint8_t(v[0])) | (uint32_t(uint8_t(v[1])) << 8)
                                          | (uint32_t(uint8_t(v[2])) << 16) | (uint32_t(uint8_t(v[3])) << 24))
                                       : (uint32_t(uint8_t(v[3])) | (uint32_t(uint8_t(v[2])) << 8)
                                          | (uint32_t(uint8_t(v[1])) << 16) | (uint32_t(uint8_t(v[0])) << 24));
            }

            /**
             * Reads a 64-bit integer from a character array in either little-endian or big-endian byte order.
             *
             * @param v The character array to read from.
             *
             * @return The 64-bit integer read from the character array.
             */
            uint64_t read64(const char* v) {
                return little_endian() ? (uint64_t(uint8_t(v[0])) | (uint64_t(uint8_t(v[1])) << 8)
                                          | (uint64_t(uint8_t(v[2])) << 16) | (uint64_t(uint8_t(v[3])) << 24)
                                          | (uint64_t(uint8_t(v[4])) << 32) | (uint64_t(uint8_t(v[5])) << 40)
                                          | (uint64_t(uint8_t(v[6])) << 48) | (uint64_t(uint8_t(v[7])) << 56))
                                       : (uint64_t(uint8_t(v[7])) | (uint64_t(uint8_t(v[6])) << 8)
                                          | (uint64_t(uint8_t(v[5])) << 16) | (uint64_t(uint8_t(v[4])) << 24)
                                          | (uint64_t(uint8_t(v[3])) << 32) | (uint64_t(uint8_t(v[2])) << 40)
                                          | (uint64_t(uint8_t(v[1])) << 48) | (uint64_t(uint8_t(v[0])) << 56));
            }

            /**
             * Rotates the bits of an integral type to the left by a given amount.
             *
             * @note This function is only enabled for integral types.
             *
             * @tparam T The integral type to rotate.
             *
             * @param x The value to rotate.
             * @param r The number of bits to rotate by.
             *
             * @return The rotated value.
             */
            template <typename T>
            std::enable_if_t<std::is_integral<T>::value, T> rotl(const T& x, const int& r) {
                return ((x << r) | (x >> (sizeof(T) * 8 - r)));
            }

        }  // namespace

        uint32_t xxhash32(const void* input_v, const size_t& length, const uint32_t& seed) {

            static constexpr uint32_t PRIME1 = 0x9E3779B1U;
            static constexpr uint32_t PRIME2 = 0x85EBCA77U;
            static constexpr uint32_t PRIME3 = 0xC2B2AE3DU;
            static constexpr uint32_t PRIME4 = 0x27D4EB2FU;
            static constexpr uint32_t PRIME5 = 0x165667B1U;

            // Cast the input pointer to a character pointer.
            const auto* input = static_cast<const char*>(input_v);
            /// The hash value being calculated.
            uint32_t h{};
            /// A pointer to the current position in the input buffer.
            const char* p = input;

            // Process in 16-byte chunks.
            if (length >= 16) {
                uint32_t v1 = seed + PRIME1 + PRIME2;
                uint32_t v2 = seed + PRIME2;
                uint32_t v3 = seed;
                uint32_t v4 = seed - PRIME1;

                for (; p < input + (length & ~0xF); p += 16) {
                    v1 = rotl(v1 + (read32(p) * PRIME2), 13) * PRIME1;
                    v2 = rotl(v2 + (read32(p + 4) * PRIME2), 13) * PRIME1;
                    v3 = rotl(v3 + (read32(p + 8) * PRIME2), 13) * PRIME1;
                    v4 = rotl(v4 + (read32(p + 12) * PRIME2), 13) * PRIME1;
                }
                // Mix
                h = rotl(v1, 1) + rotl(v2, 7) + rotl(v3, 12) + rotl(v4, 18);
            }
            else {
                // No 16-byte chunks.
                h = seed + PRIME5;
            }

            h += uint32_t(length);

            // Process in 4-byte chunks.
            for (; p < input + (length & ~0x3); p += 4) {
                h = rotl(h + (read32(p) * PRIME3), 17) * PRIME4;
            }

            // Process the remainder.
            for (; p < input + length; p++) {
                h = rotl(h + (uint8_t(*p) * PRIME5), 11) * PRIME1;
            }

            // Avalanche
            h = (h ^ (h >> 15)) * PRIME2;
            h = (h ^ (h >> 13)) * PRIME3;
            h = (h ^ (h >> 16));

            return h;
        }

        uint64_t xxhash64(const void* input_v, const size_t& length, const uint64_t& seed) {

            static constexpr uint64_t PRIME1 = 11400714785074694791ULL;
            static constexpr uint64_t PRIME2 = 14029467366897019727ULL;
            static constexpr uint64_t PRIME3 = 1609587929392839161ULL;
            static constexpr uint64_t PRIME4 = 9650029242287828579ULL;
            static constexpr uint64_t PRIME5 = 2870177450012600261ULL;

            // Cast the input pointer to a character pointer.
            const auto* input = static_cast<const char*>(input_v);
            /// The hash value being calculated.
            uint64_t h{};
            /// A pointer to the current position in the input buffer.
            const char* p = input;

            // Process 32 byte chunks if we can
            if (length >= 32) {
                uint64_t v1 = seed + PRIME1 + PRIME2;
                uint64_t v2 = seed + PRIME2;
                uint64_t v3 = seed;
                uint64_t v4 = seed - PRIME1;

                for (; p < input + (length & ~0x1F); p += 32) {
                    v1 = rotl(v1 + read64(p) * PRIME2, 31) * PRIME1;
                    v2 = rotl(v2 + read64(p + 8) * PRIME2, 31) * PRIME1;
                    v3 = rotl(v3 + read64(p + 16) * PRIME2, 31) * PRIME1;
                    v4 = rotl(v4 + read64(p + 24) * PRIME2, 31) * PRIME1;
                }
                // Mix
                h = rotl(v1, 1) + rotl(v2, 7) + rotl(v3, 12) + rotl(v4, 18);
                h = (h ^ rotl(v1 * PRIME2, 31) * PRIME1) * PRIME1 + PRIME4;
                h = (h ^ rotl(v2 * PRIME2, 31) * PRIME1) * PRIME1 + PRIME4;
                h = (h ^ rotl(v3 * PRIME2, 31) * PRIME1) * PRIME1 + PRIME4;
                h = (h ^ rotl(v4 * PRIME2, 31) * PRIME1) * PRIME1 + PRIME4;
            }
            else {
                h = seed + PRIME5;
            }

            h += length;

            // Process in 8-byte chunks.
            for (; p < input + (length & ~0x7); p += 8) {
                h = rotl(h ^ (rotl(read64(p) * PRIME2, 31) * PRIME1), 27) * PRIME1 + PRIME4;
            }

            // Process in 4-byte chunks.
            for (; p < input + (length & ~0x3); p += 4) {
                h = rotl(h ^ (read32(p) * PRIME1), 23) * PRIME2 + PRIME3;
            }

            // Process the remainder.
            for (; p < input + length; p++) {
                h = rotl(h ^ (uint8_t(*p) * PRIME5), 11) * PRIME1;
            }

            // Avalanche
            h = (h ^ (h >> 33)) * PRIME2;
            h = (h ^ (h >> 29)) * PRIME3;
            h = h ^ (h >> 32);

            return h;
        }

    }  // namespace serialise
}  // namespace util
}  // namespace NUClear
