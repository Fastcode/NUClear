/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_EXTENSIONS_SERIALIZATION_MURMURHASH3_H
#define NUCLEAR_EXTENSIONS_SERIALIZATION_MURMURHASH3_H

#include <cstdint>
#include <array>

#include <functional>

namespace NUClear {
    namespace extensions {
        namespace serialization {
            
            /**
             * @brief A 128 bit hash object used to identify types over the network
             *
             * @author Trent Houliston
             */
            struct Hash {
                /// @brief our 128 bit hash
                std::array<uint64_t, 2> data;
                
                /**
                 * @brief Check for equality between two hashes
                 *
                 * @param hash the hash to compare against
                 *
                 * @return if the hashes were equal
                 */
                bool operator==(const Hash& hash) const;
                
                /**
                 * @brief Convert this hash into a hash for use in a hash table (std hash)
                 *
                 * @return the std hash for this hash
                 */
                size_t hash() const;
            };
            
            /**
             * @brief Constructs a new hash from the passed key (of length bytes)
             *
             * @param key a pointer to the data for the key
             * @param len the number of bytes in the key
             */
            Hash murmurHash3(const void* key, const size_t len);
        }
    }
}

namespace std {
    template <>
    struct hash<NUClear::extensions::serialization::Hash> : public unary_function<NUClear::extensions::serialization::Hash, size_t> {
        
        /**
         * @brief Hashes a Hash object to an std hash
         *
         * @return the std hash
         */
        size_t operator()(const NUClear::extensions::serialization::Hash& v) const {
            return v.hash();
        }
    };
}

#endif
