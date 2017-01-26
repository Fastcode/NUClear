/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_UTIL_SERIALISE_MURMURHASH3_HPP
#define NUCLEAR_UTIL_SERIALISE_MURMURHASH3_HPP

#include <array>
#include <cstdint>

#include <functional>

namespace NUClear {
namespace util {
    namespace serialise {

        /**
         * @brief Constructs a new hash from the passed key (of length bytes)
         *
         * @details Gets the hash digest of the passed bytes using the MurmurHash3
         *  hashing function. Specifically the x64 128 bit version using the seed
         *  0x4e55436c
         *
         * @param key a pointer to the data for the key
         * @param len the number of bytes in the key
         */
        std::array<uint64_t, 2> murmurhash3(const void* key, const size_t len);

    }  // namespace serialise
}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_SERIALISE_MURMURHASH3_HPP
