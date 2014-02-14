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

#include "nuclear_bits/extensions/serialization/MurmurHash3.h"

namespace NUClear {
    namespace extensions {
        namespace serialization {
            
            bool Hash::operator==(const Hash& hash) const {
                return data == hash.data;
            }
            
            inline uint64_t rotl64(const uint64_t x, const int8_t r)
            {
                return (x << r) | (x >> (64 - r));
            }
            
            Hash murmurHash3(const void* key, const size_t len) {
                
                // Work out how many blocks we are going to use
                const int nblocks = len / 16;
                
                // Magic number seeds (NUClear in hex)
                uint64_t h1 = 0x4e55436c;
                uint64_t h2 = 0x4e55436c;
                
                // Murmurhash3 magic numbers
                const uint64_t c1 = 0x87c37b91114253d5;
                const uint64_t c2 = 0x4cf5ad432745937f;
                
                // Cast our data as blocks and bytes
                const uint64_t* blocks = static_cast<const uint64_t*>(key);
                const uint8_t* data = static_cast<const uint8_t*>(key);
                
                // Perform the hashing for each block
                for(int i = 0; i < nblocks; ++i)
                {
                    uint64_t k1 = blocks[i*2+0];
                    uint64_t k2 = blocks[i*2+1];
                    
                    k1 *= c1;
                    k1  = rotl64(k1,31);
                    k1 *= c2;
                    h1 ^= k1;
                    
                    h1  = rotl64(h1,27);
                    h1 += h2;
                    h1  = h1 * 5 + 0x52dce729;
                    
                    k2 *= c2;
                    k2  = rotl64(k2,33);
                    k2 *= c1;
                    h2 ^= k2;
                    
                    h2  = rotl64(h2,31);
                    h2 += h1;
                    h2  = h2 * 5 + 0x38495ab5;
                }
                
                // Hash our tail block
                const uint8_t * tail = static_cast<const uint8_t*>(data + nblocks * 16);
                
                uint64_t k1 = 0;
                uint64_t k2 = 0;
                
                switch(len & 15)
                {
                    case 15: k2 ^= ((uint64_t)tail[14]) << 48;
                    case 14: k2 ^= ((uint64_t)tail[13]) << 40;
                    case 13: k2 ^= ((uint64_t)tail[12]) << 32;
                    case 12: k2 ^= ((uint64_t)tail[11]) << 24;
                    case 11: k2 ^= ((uint64_t)tail[10]) << 16;
                    case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
                    case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
                        k2 *= c2;
                        k2  = rotl64(k2,33);
                        k2 *= c1;
                        h2 ^= k2;
                        
                    case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
                    case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
                    case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
                    case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
                    case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
                    case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
                    case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
                    case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
                        k1 *= c1;
                        k1  = rotl64(k1,31);
                        k1 *= c2;
                        h1 ^= k1;
                };
                
                // Hash finalization
                h1 ^= len;
                h2 ^= len;
                
                h1 += h2;
                h2 += h1;
                
                h1 ^= h1 >> 33;
                h1 *= 0xff51afd7ed558ccd;
                h1 ^= h1 >> 33;
                h1 *= 0xc4ceb9fe1a85ec53;
                h1 ^= h1 >> 33;
                
                h2 ^= h2 >> 33;
                h2 *= 0xff51afd7ed558ccd;
                h2 ^= h2 >> 33;
                h2 *= 0xc4ceb9fe1a85ec53;
                h2 ^= h2 >> 33;
                
                h1 += h2;
                h2 += h1;
                
                Hash ret;
                memcpy(&ret.data, &h1, sizeof(uint64_t));
                memcpy(&ret.data + sizeof(uint64_t), &h2, sizeof(uint64_t));
                
                return ret;
            }
            
            size_t Hash::hash() const {
                std::hash<std::bitset<128>> hasher;
                return hasher(data);
            }
        }
    }
}
