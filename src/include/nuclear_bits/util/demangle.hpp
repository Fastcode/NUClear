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

#ifndef NUCLEAR_UTIL_DEMANGLE_H
#define NUCLEAR_UTIL_DEMANGLE_H

// Windows symbol demangler
#ifdef _MSC_VER

namespace NUClear {
    namespace util {
        inline std::string demangle(const char* symbol) {
            return symbol;
        }
    }
}

// GNU/Clang symbol demangler
#else

#include <cxxabi.h>

namespace NUClear {
    namespace util {

        /**
         * @brief Demangles the passed symbol to a string, or returns it if it cannot demangle it
         *
         * @param symbol the symbol to demangle
         *
         * @return the demangled symbol, or the original string if it could not be demangeld
         */
        inline std::string demangle(const char* symbol) {

            int status = -4; // some arbitrary value to eliminate the compiler warning
            std::unique_ptr<char, void(*)(void*)> res {
                abi::__cxa_demangle(symbol, nullptr, nullptr, &status),
                std::free
            };

            return std::string(status == 0 ? res.get() : symbol);
        }
    }
}
#endif  // _MSC_VER

#endif  // NUCLEAR_UTIL_DEMANGLE_H
