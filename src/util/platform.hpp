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

#ifndef NUCLEAR_UTIL_PLATFORM_HPP
#define NUCLEAR_UTIL_PLATFORM_HPP

/*******************************************
 *   MANAGE WINDOWS CRAZY INCLUDE SYSTEM   *
 *******************************************/
// Because windows is SUUUUPER dumb and if you include headers in the wrong order
// Nothing at all works, also if you don't define things in the right order nothing works
// It's a terrible pile of garbage
// So we have this header to make sure everything is in the correct order
#ifdef _WIN32

#    include <SdkDdkver.h>

// We need at least windows vista so functions like inet_ntop exist
#    ifndef NTDDI_VERSION
#        define NTDDI_VERSION NTDDI_VISTA
#    endif

#    ifndef WINVER
#        define WINVER _WIN32_WINNT_VISTA
#    endif

#    ifndef _WIN32_WINNT
#        define _WIN32_WINNT _WIN32_WINNT_VISTA
#    endif

// Without this winsock just doesn't have half the typedefs
#    ifndef INCL_WINSOCK_API_TYPEDEFS
#        define INCL_WINSOCK_API_TYPEDEFS 1
#    endif

// Windows has a dumb min/max macro that breaks stuff
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif

// Winsock must be declared before Windows.h or it won't work
// clang-format off
#    include <WinSock2.h>

#    include <Ws2ipdef.h>
#    include <Ws2tcpip.h>

#    include <Mstcpip.h>
#    include <Mswsock.h>

#    include <Iphlpapi.h>
// clang-format on

// This little thingy makes windows link to the winsock library
#    pragma comment(lib, "Ws2_32.lib")
#    pragma comment(lib, "Mswsock.lib")
#    pragma comment(lib, "IPHLPAPI.lib")

// Include windows.h mega header... no wonder windows compiles so slowly
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>

// Whoever thought this was a good idea was a terrible person
#    undef ERROR

#endif  // _WIN32

/*******************************************
 *      SHIM FOR THREAD LOCAL STORAGE      *
 *******************************************/
#if defined(__GNUC__)
#    define ATTRIBUTE_TLS __thread
#elif defined(_WIN32)
#    define ATTRIBUTE_TLS __declspec(thread)
#else  // !__GNUC__ && !_MSC_VER
#    error "Define a thread local storage qualifier for your compiler/platform!"
#endif

/*******************************************
 *           SHIM FOR NETWORKING           *
 *******************************************/
#ifdef _WIN32

#    include <cstdint>
using ssize_t   = SSIZE_T;
using in_port_t = uint16_t;
using in_addr_t = uint32_t;

namespace NUClear {

// For us file descriptors will just be sockets
using fd_t = SOCKET;

using socklen_t = int;

// This is defined here rather than in the global namespace so it doesn't get in the way
inline int close(fd_t fd) {
    return ::closesocket(fd);
}

inline int ioctl(SOCKET s, long cmd, u_long* argp) {
    return ioctlsocket(s, cmd, argp);
}

// Network errors come from WSAGetLastError()
#    define network_errno WSAGetLastError()

// Make iovec into a windows WSABUF
#    define iovec WSABUF
#    define iov_base buf
#    define iov_len len

// Make msghdr into WSAMSG
#    define msghdr WSAMSG
#    define msg_name name
#    define msg_namelen namelen
#    define msg_iov lpBuffers
#    define msg_iovlen dwBufferCount
#    define msg_control Control.buf
#    define msg_controllen Control.len
#    define msg_flags flags

// Reimplement the recvmsg function
int recvmsg(fd_t fd, msghdr* msg, int flags);

int sendmsg(fd_t fd, msghdr* msg, int flags);
}  // namespace NUClear

#else

// Include real networking stuff
#    include <arpa/inet.h>
#    include <ifaddrs.h>
#    include <net/if.h>
#    include <netdb.h>
#    include <netinet/in.h>
#    include <poll.h>
#    include <sys/ioctl.h>
#    include <sys/socket.h>
#    include <sys/types.h>
#    include <unistd.h>

// Move errno so it can be used in windows
#    define network_errno errno
#    define INVALID_SOCKET -1

namespace NUClear {
using fd_t = int;
}  // namespace NUClear

#endif

#endif  // NUCLEAR_UTIL_PLATFORM_HPP
