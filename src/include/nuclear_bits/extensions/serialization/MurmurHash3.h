/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
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
#include <cstring>

#include <functional>

namespace NUClear {
    namespace extensions {
        namespace serialization {
            
            struct Hash {
                static const size_t SIZE = 16;
                uint8_t data[SIZE];
                
                bool operator==(const Hash& hash) const;
                size_t hash() const;
                static size_t hashToStdHash(const uint8_t* data);
            };
            
            Hash murmurHash3(const void* key, const size_t len);
        }
    }
}

namespace std {
    template <>
    struct hash<NUClear::extensions::serialization::Hash> : public unary_function<NUClear::extensions::serialization::Hash, size_t> {
        
        size_t operator()(const NUClear::extensions::serialization::Hash& v) const {
            return v.hash();
        }
    };
}

#endif
