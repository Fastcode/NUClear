/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#ifndef NUCLEAR_UTIL_FILEDESCRIPTOR_HPP
#define NUCLEAR_UTIL_FILEDESCRIPTOR_HPP

#include "platform.hpp"

namespace NUClear {
namespace util {

    /**
     * @brief An RAII file descriptor.
     * @details This class represents an RAII file descriptor.
     *          It will close the file descriptor it holds on
     *          destruction.
     */
    struct FileDescriptor {

        /**
         * @brief Constructs a new RAII file descriptor.
         *
         * @param fd [description]
         */
        FileDescriptor(fd_t fd) : fd(fd) {}

        /**
         * @brief Destruct the file descriptor, closes the held FD
         */
        ~FileDescriptor() {
// On windows it's CLOSE_SOCKET
#if defined(_WIN32)
            if (fd != INVALID_SOCKET) { close(fd); }
// On others we just close
#else
            if (fd > 0) { close(fd); }
#endif
        }

        /**
         * @brief Releases the file descriptor from RAII.
         * @details This releases and returns the file descriptor
         *          it holds. This allows it to no longer be
         *          subject to closure on deletion.
         *          After this this FileDescriptor object will
         *          be invalidated.
         * @return the held file descriptor
         */
        fd_t release() {
            fd_t temp = fd;
            fd        = -1;
            return temp;
        }

        /**
         * @brief Casts this to the held file descriptor
         *
         * @return the file descriptor
         */
        operator fd_t() {
            return fd;
        }

        /// @brief The held file descriptor
        fd_t fd;
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_FILEDESCRIPTOR_HPP
