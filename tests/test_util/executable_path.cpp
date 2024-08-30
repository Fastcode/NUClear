/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#include "executable_path.hpp"

#include <array>
#include <climits>
#include <stdexcept>
#include <string>
#include <system_error>

#if defined(_WIN32)
    #include "util/platform.hpp"

namespace test_util {
std::string get_executable_path() {
    std::array<char, MAX_PATH> buffer;
    const DWORD size = GetModuleFileName(NULL, buffer.data(), buffer.size());
    if (size) {
        return {buffer.data(), size};
    }
    throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory),
                            "Could not get executable path");
}
}  // namespace test_util

#elif defined(__APPLE__)
    #include <mach-o/dyld.h>

namespace test_util {
std::string get_executable_path() {
    std::array<char, PATH_MAX> buffer{};
    uint32_t size = buffer.size();
    if (::_NSGetExecutablePath(buffer.data(), &size) == 0) {
        return {buffer.data(), size};
    }
    throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory),
                            "Could not get executable path");
}
}  // namespace test_util

#elif defined(__linux__)
    #include <unistd.h>

namespace test_util {
std::string get_executable_path() {
    std::array<char, PATH_MAX> buffer{};
    const ssize_t size = ::readlink("/proc/self/exe", buffer.data(), buffer.size());
    if (size >= 1) {
        return {buffer.data(), size_t(size)};
    }
    throw std::system_error(std::make_error_code(std::errc::no_such_file_or_directory),
                            "Could not get executable path");
}
}  // namespace test_util

#else
    #error "Unsupported platform"
#endif
