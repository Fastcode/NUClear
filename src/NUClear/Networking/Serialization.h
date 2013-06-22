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

#include "NUClear/PowerPlant.h"

namespace NUClear {
    namespace Networking {
        
        struct Hash {
            uint8_t* data;
            size_t size;
            size_t stdhash;
            
            bool operator==(const Hash& hash) const;
        };
        
        template <typename TType>
        struct Serializer {
            
            static TType* deserialize(void* data) {
                return static_cast<TType*>(data);
            }
            
            static std::pair<std::shared_ptr<void>, size_t> serialize(TType* data) {
                
                // Make a shared pointer (so it is deallocated correctly)
                std::shared_ptr<TType> ptr(data);
                
                return std::make_pair(std::static_pointer_cast<void>(ptr), sizeof(TType));
            }
        };
        
        template <typename TType>
        Hash hash() {
            
            // Set all the bytes to the first byte of the mangled name (TODO use murmur3 or something to hash better)
            uint8_t* data = new uint8_t[8] {
                static_cast<uint8_t>(typeid(TType).name()[0])
            };
            
            return Hash{data, 8};
        }
    }
}

namespace std
{
    template <>
    struct hash<NUClear::Networking::Hash> : public unary_function<NUClear::Networking::Hash, size_t>
    {
        size_t operator()(const NUClear::Networking::Hash& v) const
        {
            return v.stdhash;
        }
    };
}
