/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_FILEDESCRIPTOR_HPP
#define NUCLEAR_UTIL_FILEDESCRIPTOR_HPP

#include <functional>

#include "platform.hpp"

namespace NUClear {
namespace util {

    /**
     * An RAII file descriptor.
     *
     * This class represents an RAII file descriptor.
     * It will close the file descriptor it holds on destruction.
     */
    class FileDescriptor {
    public:
        /**
         * Construct a File Descriptor with an invalid file descriptor.
         */
        FileDescriptor();

        /**
         * Constructs a new RAII file descriptor.
         *
         * @param fd the file descriptor to hold
         * @param cleanup an optional cleanup function to call on close
         */
        FileDescriptor(const fd_t& fd_, std::function<void(fd_t)> cleanup_ = nullptr);

        // Don't allow copy construction or assignment
        FileDescriptor(const FileDescriptor&)            = delete;
        FileDescriptor& operator=(const FileDescriptor&) = delete;

        // Allow move construction or assignment
        FileDescriptor(FileDescriptor&& rhs) noexcept;
        FileDescriptor& operator=(FileDescriptor&& rhs) noexcept;

        /**
         * Destruct the file descriptor, closes the held fd.
         */
        ~FileDescriptor();

        /**
         * Get the currently held file descriptor.
         *
         * @return the file descriptor
         */
        // No Lint: As we are giving access to a variable which can change state.
        // NOLINTNEXTLINE(readability-make-member-function-const)
        fd_t get();

        /**
         * Returns if the currently held file descriptor is valid.
         *
         * @return `true` if the file descriptor is valid
         */
        bool valid() const;

        /**
         * Close the currently held file descriptor.
         */
        void close();

        /**
         * Release the currently held file descriptor.
         *
         * @return The file descriptor
         */
        fd_t release();

        /**
         * Implicitly convert this class to a file descriptor.
         *
         * @return The file descriptor
         */
        operator fd_t();

    private:
        /// The held file descriptor
        fd_t fd{INVALID_SOCKET};
        /// An optional cleanup function to call on close
        std::function<void(fd_t)> cleanup;
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_FILEDESCRIPTOR_HPP
