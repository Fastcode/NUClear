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

#ifndef NUCLEAR_EXTENSION_SERIALISE_XXHASH_HPP
#define NUCLEAR_EXTENSION_SERIALISE_XXHASH_HPP

#include <cstddef>
#include <cstdint>

namespace NUClear {
namespace util {
    namespace serialise {

        /**
         * Calculates the 32-bit xxhash of the input data.
         *
         * This function calculates the 32-bit xxhash of the input data using the specified seed value.
         *
         * @param input  Pointer to the input data.
         * @param length Length of the input data in bytes.
         * @param seed   Seed value to use for the hash calculation. Default value is 0.
         *
         * @return The 32-bit xxHash of the input data.
         */
        uint32_t xxhash32(const char* input, const size_t& length, const uint32_t& seed = 0);

        template <typename T>
        uint32_t xxhash32(const T* input, const uint32_t& seed = 0) {
            return xxhash32(reinterpret_cast<const char*>(input), sizeof(T), seed);
        }

        /**
         * Calculates the 64-bit xxhash of the input data.
         *
         * This function calculates the 64-bit xxhash of the input data using the specified seed value.
         *
         * @param input  Pointer to the input data.
         * @param length Length of the input data in bytes.
         * @param seed   Seed value to use for the hash calculation. Default value is 0.
         *
         * @return The 64-bit xxHash of the input data.
         */
        uint64_t xxhash64(const char* input, const size_t& length, const uint64_t& seed = 0);

        template <typename T>
        uint64_t xxhash64(const T* input, const uint64_t& seed = 0) {
            return xxhash64(reinterpret_cast<const char*>(input), sizeof(T), seed);
        }

    }  // namespace serialise
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_SERIALISE_XXHASH_HPP
