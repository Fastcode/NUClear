/*
 * MIT License
 *
 * Copyright (c) 2016 NUClear Contributors
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

#include "demangle.hpp"

#include <regex>

// Windows symbol demangler
#ifdef _WIN32

    #include "platform.hpp"

// Dbghelp.h depends on types from Windows.h so it needs to be included first
// Separate to always include platform.hpp (which includes windows.h) first

    #include <Dbghelp.h>

    #include <array>
    #include <mutex>

    #pragma comment(lib, "Dbghelp.lib")

namespace NUClear {
namespace util {

    bool sym_initialised = false;
    std::mutex symbol_mutex;

    void init_symbols() {

        HANDLE h_process;

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

        h_process = GetCurrentProcess();

        if (!SymInitialize(h_process, nullptr, true)) {
            // SymInitialize failed
            throw std::system_error(GetLastError(), std::system_category(), "SymInitialise failed");
        }

        sym_initialised = true;
    }

    std::string demangle(const char* symbol) noexcept {
        // If the symbol is null or the empty string then just return it
        if (symbol == nullptr || symbol[0] == '\0') {
            return "";
        }

        const std::lock_guard<std::mutex> lock(symbol_mutex);

        // Initialise the symbols if we have to
        if (!sym_initialised) {
            init_symbols();
        }

        std::array<char, 256> name{};
        auto len = UnDecorateSymbolName(symbol, name.data(), DWORD(name.size()), 0);

        if (len > 0) {
            std::string demangled(name.data(), len);
            demangled = std::regex_replace(demangled, std::regex(R"(struct\s+)"), "");
            demangled = std::regex_replace(demangled, std::regex(R"(class\s+)"), "");
            demangled = std::regex_replace(demangled, std::regex(R"(\s+)"), "");
            return demangled;
        }
        else {
            return symbol;
        }
    }
}  // namespace util
}  // namespace NUClear

// GNU/Clang symbol demangler
#else

    #include <cxxabi.h>  // for __cxa_demangle

    #include <cstdlib>  // for free
    #include <memory>   // for unique_ptr
    #include <string>   // for string

namespace NUClear {
namespace util {

    /**
     * Demangles the passed symbol to a string, or returns it if it cannot demangle it.
     *
     * @param symbol the symbol to demangle
     *
     * @return the demangled symbol, or the original string if it could not be demangled
     */
    std::string demangle(const char* symbol) noexcept {

        if (symbol == nullptr) {
            return {};
        }

        int status = -1;
        const std::unique_ptr<char, void (*)(char*)> res{abi::__cxa_demangle(symbol, nullptr, nullptr, &status),
                                                         [](char* ptr) {
                                                             // The API for __cxa_demangle REQUIRES us to call free
                                                             // No choice here so suppress the linter
                                                             // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
                                                             std::free(ptr);  // NOSONAR
                                                         }};
        if (res != nullptr) {
            std::string demangled = res.get();
            demangled             = std::regex_replace(demangled, std::regex(R"(\s+)"), "");
            return demangled;
        }

        return symbol;
    }

}  // namespace util
}  // namespace NUClear

#endif  // _MSC_VER
