/*
     * MIT License
     *
     * Copyright (c) 2025 NUClear Contributors
     *
     * This file is part of the NUClear codebase.
     * See https://github.com/Fastcode/NUClear for further info.
     *
     * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
     * documentation files (the "Software"), to deal in the Software without restriction, including without limitation
     * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and
     * to permit persons to whom the Software is furnished to do so, subject to the following conditions:
     *
     * The above copyright notice and this permission notice shall be included in all copies or substantial portions of
     * the Software.
     *
     * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
     * THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
     * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
     * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
     * IN THE SOFTWARE.
     */

#ifndef NUCLEAR_NETWORK_FILE_DESCRIPTOR_HPP
#define NUCLEAR_NETWORK_FILE_DESCRIPTOR_HPP

#include "../util/platform.hpp"

    namespace NUClear {
    namespace network {

        /**
         * RAII wrapper for a file descriptor (socket).
         * Automatically closes the file descriptor on destruction.
         * Non-copyable, move-only.
         */
        class FileDescriptor {
        public:
            /// Construct with an invalid descriptor
            FileDescriptor() = default;

            /// Construct taking ownership of an existing file descriptor
            explicit FileDescriptor(fd_t fd) : fd(fd) {}

            /// Destructor closes the descriptor if valid
            ~FileDescriptor() {
                reset();
            }

            // Non-copyable
            FileDescriptor(const FileDescriptor&)            = delete;
            FileDescriptor& operator=(const FileDescriptor&) = delete;

            // Movable
            FileDescriptor(FileDescriptor&& other) noexcept : fd(other.fd) {
                other.fd = INVALID_SOCKET;
            }
            FileDescriptor& operator=(FileDescriptor&& other) noexcept {
                if (this != &other) {
                    reset();
                    fd       = other.fd;
                    other.fd = INVALID_SOCKET;
                }
                return *this;
            }

            /// Get the raw file descriptor
            fd_t get() const {
                return fd;
            }

            /// Check if the descriptor is valid
            bool valid() const {
                return fd != INVALID_SOCKET;
            }

            /// Implicit conversion to fd_t for use with system calls
            operator fd_t() const {  // NOLINT(google-explicit-constructor)
                return fd;
            }

            /// Release ownership without closing
            fd_t release() {
                fd_t old = fd;
                fd       = INVALID_SOCKET;
                return old;
            }

            /// Close the current descriptor and take ownership of a new one
            void reset(fd_t new_fd = INVALID_SOCKET) {
                if (fd != INVALID_SOCKET) {
                    ::close(fd);
                }
                fd = new_fd;
            }

        private:
            fd_t fd{INVALID_SOCKET};
        };

    }  // namespace network
}  // namespace NUClear

#endif  // NUCLEAR_NETWORK_FILE_DESCRIPTOR_HPP
