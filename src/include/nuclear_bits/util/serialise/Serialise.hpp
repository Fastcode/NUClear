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

#ifndef NUCLEAR_UTIL_SERIALISE_SERIALISE_H
#define NUCLEAR_UTIL_SERIALISE_SERIALISE_H

#include <string>

#include "nuclear_bits/util/demangle.hpp"
#include "nuclear_bits/util/serialise/MurmurHash3.hpp"
#include "nuclear_bits/util/MetaProgramming.hpp"

namespace NUClear {
    namespace util {
        namespace serialise {
            
            // Import metaprogramming tools
            template <typename Condition, typename Value>
            using EnableIf = util::Meta::EnableIf<Condition, Value>;
            
            template <typename T, typename Check = T>
            struct Serialise;
            
            // Plain old data
            template <typename T>
            struct Serialise<T, EnableIf<std::is_pod<T>, T>> {
                
                static inline std::vector<char> serialise(const T& in) {
                    
                    // Copy the bytes into an array
                    const char* dataptr = reinterpret_cast<const char*>(&in);
                    return std::vector<char>(dataptr, dataptr + sizeof(T));
                }
                
                static inline T deserialise(const std::vector<char>& in) {
                    
                    // Copy the data into an object of the correct type
                    T ret = *reinterpret_cast<T*>(in.data());
                    return ret;
                }
                
                static inline std::array<uint64_t, 2> hash() {
                    
                    // Serialise based on the demangled class name
                    std::string typeName = demangle(typeid(T).name());
                    return murmurHash3(typeName.c_str(), typeName.size());
                }
            };
            
            // Iterable of pod
            template <typename T>
            struct Serialise<T, EnableIf<std::is_same<typename std::iterator_traits<decltype(std::declval<T>().begin())>::iterator_category, std::random_access_iterator_tag>, T>> {
                
                using StoredType = Meta::RemoveRef<decltype(*std::declval<T>().begin())>;
                
                static inline std::vector<char> serialise(const T& in) {
                    std::vector<char> out;
                    out.reserve(std::distance(in.begin(), in.end()));
                    
                    for(const StoredType& item : in) {
                        const char* i = reinterpret_cast<const char*>(&item);
                        out.insert(out.end(), i, i + sizeof(decltype(item)));
                    }
                    
                    return out;
                }
                
                static inline T deserialise(const std::vector<char>& in) {
                    
                    T out;
                    
                    const StoredType* data = reinterpret_cast<const StoredType*>(in.data());
                    
                    out.insert(out.end(), data, data + (in.size() / sizeof(StoredType)));
                               
                   return out;
                }
                
                static inline std::array<uint64_t, 2> hash() {
                    
                    // Serialise based on the demangled class name
                    std::string typeName = demangle(typeid(T).name());
                    return murmurHash3(typeName.c_str(), typeName.size());
                }
            };
        }
    }
}

#endif
