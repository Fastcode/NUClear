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
    class FileDescriptor {

        /**
         * @brief Constructs a new RAII file descriptor.
         *
         * @param fd [description]
         */

    public:
        FileDescriptor();
        FileDescriptor(const fd_t& fd);

        // Don't allow copy construction or assignment
        FileDescriptor(const FileDescriptor&) = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;

        // Allow move construction or assignment
        FileDescriptor(FileDescriptor&& rhs) noexcept;
        FileDescriptor& operator=(FileDescriptor&& rhs) noexcept;

        /**
         * @brief Destruct the file descriptor, closes the held FD
         */
        ~FileDescriptor();

        /**
         * @brief Get the currently held file descriptor
         *
         * @return the file descriptor
         */
        // No Lint: As we are giving access to a variable which can change state.
        // NOLINTNEXTLINE(readability-make-member-function-const)
        fd_t get();

        /**
         * @brief Returns if the currently held file descriptor is valid
         *
         * @return true     if the file descriptor is valid
         * @return false    if the file descriptor is invalid
         */
        bool valid() const;

        /**
         * @brief Releases the file descriptor from RAII.
         * @details This releases and returns the file descriptor
         *          it holds. This allows it to no longer be
         *          subject to closure on deletion.
         *          After this this FileDescriptor object will
         *          be invalidated.
         * @return the held file descriptor
         */
        fd_t release();

        /**
         * @brief Casts this to the held file descriptor
         *
         * @return the file descriptor
         */
        operator fd_t();

        /**
         * @brief Close the current file descriptor, if it's valid
         */
        void close_fd();

    private:
        /// @brief The held file descriptor
        fd_t fd{INVALID_SOCKET};
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_FILEDESCRIPTOR_HPP
