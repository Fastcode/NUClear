/*
 * MIT License
 *
 * Copyright (c) 2021 NUClear Contributors
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

#include "FileDescriptor.hpp"

#include <functional>
#include <utility>

#include "platform.hpp"

namespace NUClear {
namespace util {


    FileDescriptor::FileDescriptor() = default;
    FileDescriptor::FileDescriptor(const fd_t& fd_, std::function<void(fd_t)> cleanup_)
        : fd(fd_), cleanup(std::move(cleanup_)) {}

    FileDescriptor::~FileDescriptor() {
        close();
    }

    void FileDescriptor::close() {
        if (valid()) {
            if (cleanup) {
                cleanup(fd);
            }
            ::close(fd);
            fd = INVALID_SOCKET;
        }
    }

    FileDescriptor::FileDescriptor(FileDescriptor&& rhs) noexcept {
        if (this != &rhs) {
            fd      = std::exchange(rhs.fd, INVALID_SOCKET);
            cleanup = std::move(rhs.cleanup);
        }
    }

    FileDescriptor& FileDescriptor::operator=(FileDescriptor&& rhs) noexcept {
        if (this != &rhs) {
            this->close();
            fd      = std::exchange(rhs.fd, INVALID_SOCKET);
            cleanup = std::move(rhs.cleanup);
        }
        return *this;
    }

    // No Lint: As we are giving access to a variable which can change state.
    // NOLINTNEXTLINE(readability-make-member-function-const)
    fd_t FileDescriptor::get() {
        return fd;
    }

    bool FileDescriptor::valid() const {
#ifdef _WIN32
        return fd != INVALID_SOCKET;
#else
        return ::fcntl(fd, F_GETFL) != -1 || errno != EBADF;
#endif
    }

    fd_t FileDescriptor::release() {
        return std::exchange(fd, INVALID_SOCKET);
    }

    // Should not be const as editing the file descriptor would change the state
    // NOLINTNEXTLINE(readability-make-member-function-const)
    FileDescriptor::operator fd_t() {
        return fd;
    }

}  // namespace util
}  // namespace NUClear
