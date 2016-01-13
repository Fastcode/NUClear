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

#ifndef NUCLEAR_UTIL_PLATFORM_H
#define NUCLEAR_UTIL_PLATFORM_H

/*******************************************
 *      SHIM FOR THREAD LOCAL STORAGE      *
 *******************************************/
#if defined (__GNUC__)
    #define ATTRIBUTE_TLS __thread
#elif defined (_WIN32)
    #define ATTRIBUTE_TLS __declspec(thread)
#else // !__GNUC__ && !_MSC_VER
    #error "Define a thread local storage qualifier for your compiler/platform!"
#endif

/*******************************************
 *           SHIM FOR NETWORKING           *
 *******************************************/
#ifdef _WIN32

#include "nuclear_bits/util/windows_includes.hpp"
#include <iostream>

using ssize_t = SSIZE_T;

namespace NUClear {

	// For us file descriptors will just be sockets
    using fd_t = SOCKET;

	using socklen_t = int;

    // This is defined here rather than in the global namespace so it doesn't get in the way
    inline int close(fd_t fd) {
        return ::closesocket(fd);
    }

	// Network errors come from WSAGetLastError()
	#define network_errno WSAGetLastError()

    // Make iovec into a windows WSABUF
    #define iovec WSABUF
    #define iov_base buf
    #define iov_len len

    // Make msghdr into WSAMSG
    #define msghdr WSAMSG
    #define msg_name        name
    #define msg_namelen     namelen
    #define msg_iov         lpBuffers
    #define msg_iovlen      dwBufferCount
    #define msg_control     Control.buf
    #define msg_controllen  Control.len
    #define msg_flags       flags

    // Reimplement the recvmsg function
	int recvmsg(fd_t fd, msghdr* msg, int flags);

	int sendmsg(fd_t fd, msghdr* msg, int flags);
}

#else

// Move errno so it can be used in windows
#define network_errno errno

namespace NUClear {

    using fd_t = int;
}

#endif

#endif  // NUCLEAR_UTIL_PLATFORM_H