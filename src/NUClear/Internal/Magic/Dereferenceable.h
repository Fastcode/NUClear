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

#ifndef NUCLEAR_INTERNAL_MAGIC_DEREFERENCEABLE_H
#define NUCLEAR_INTERNAL_MAGIC_DEREFERENCEABLE_H

#include <type_traits>

namespace NUClear {
namespace Internal {
namespace Magic {
    
    // Anonymous Namespace to hide implementation details
    namespace {
        // This uses SFINAE to work out if the dereference operator exists, if it does not then this doTest will fail
        // substitution and not be used, resulting in the other option being used (which does not have a void return type)
        template<class T>
        static auto doTest(int) -> decltype(*std::declval<T>(), void());
        
        // As 0 is an int, it will attempt to match the above doTest first before this one (with a return type of void)
        template<class>
        static char doTest(char);
    }
    
    /**
     * @brief TODO
     *
     * @details
     *  TODO
     */
    template<class T, bool Result = std::is_void<decltype(doTest<T>(0))>::value>
    struct Dereferenceable;
    
    template<typename T>
    struct Dereferenceable<T, true> {
        static auto dereference(T data) -> decltype(*data) {
            return *data;
        }
    };
    
    template<typename T>
    struct Dereferenceable<T, false> {
        static T dereference(T data) {
            return std::move(data);
        }
    };
}
}
}

#endif