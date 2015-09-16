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

#include "nuclear_bits/util/MetaProgramming.hpp"

namespace NUClear {
    namespace util {
        namespace serialise {
            
            // Import metaprogramming tools
            template <typename Condition, typename Value>
            using EnableIf = util::Meta::EnableIf<Condition, Value>;

            template <typename TData>
            struct Serialise {
                
                // enable if is pod
                template <typename T>
                static inline EnableIf<std::is_pod<T>, std::string> serialise(const T& in) {

                    const char* data = reinterpret_cast<const char*>(&in);
                    
                    return std::string(data, sizeof(T));
                }
                
                static inline std::string serialise(const std::string& in) {
                    return in;
                }
                
                
                // enable if is iterable of type pod
                
                // enable if is protocol buffer
            };
        }
    }
}

#endif
