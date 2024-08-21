/*
 * MIT License
 *
 * Copyright (c) 2018 NUClear Contributors
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

    #include <SdkDdkver.h>

    // We need at least windows vista so functions like inet_ntop exist
    #ifndef NTDDI_VERSION
        #define NTDDI_VERSION NTDDI_VISTA
    #endif

    #ifndef WINVER
        #define WINVER _WIN32_WINNT_VISTA
    #endif

    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT _WIN32_WINNT_VISTA
    #endif

    // Without this winsock just doesn't have half the typedefs
    #ifndef INCL_WINSOCK_API_TYPEDEFS
        #define INCL_WINSOCK_API_TYPEDEFS 1
    #endif

    // Windows has a dumb min/max macro that breaks stuff
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif

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
    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "Mswsock.lib")
    #pragma comment(lib, "IPHLPAPI.lib")

    // Include windows.h mega header... no wonder windows compiles so slowly
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>

    // Whoever thought this was a good idea was a terrible person
    #undef ERROR
    #undef RELATIVE
    #undef ABSOLUTE

    // Make the windows shutdown functions look like the posix ones
    #define SHUT_RD   SD_RECEIVE
    #define SHUT_WR   SD_SEND
    #define SHUT_RDWR SD_BOTH

    // Windows always wanting to be different
    #ifndef IPV6_RECVPKTINFO
        #define IPV6_RECVPKTINFO IPV6_PKTINFO
    #endif

#endif  // _WIN32

/*******************************************
 *      SHIM FOR THREAD LOCAL STORAGE      *
 *******************************************/
#if defined(__GNUC__)
    #define ATTRIBUTE_TLS __thread
#elif defined(_WIN32)
    #define ATTRIBUTE_TLS __declspec(thread)
#else  // !__GNUC__ && !_MSC_VER
    #error "Define a thread local storage qualifier for your compiler/platform!"
#endif

/*******************************************
 *           SHIM FOR NETWORKING           *
 *******************************************/
#if defined(__APPLE__) && defined(__MACH__)
    // On OSX the IPV6_RECVPKTINFO and IPV6_PKTINFO must be enabled using this define
    #define __APPLE_USE_RFC_3542
#endif
#ifdef _WIN32

    #include <cstdint>

using ssize_t   = SSIZE_T;
using in_port_t = uint16_t;
using in_addr_t = uint32_t;

// Make close call closesocket
inline int close(SOCKET fd) {
    return ::closesocket(fd);
}

inline int ioctl(SOCKET s, long cmd, u_long* argp) {
    return ioctlsocket(s, cmd, argp);
}

namespace NUClear {

// For us file descriptors will just be sockets
using fd_t = SOCKET;

using socklen_t = int;

    // Network errors come from WSAGetLastError()
    #define network_errno WSAGetLastError()

    // Make iovec into a windows WSABUF
    #define iovec    WSABUF
    #define iov_base buf
    #define iov_len  len

    // Make msghdr into WSAMSG
    #define msghdr         WSAMSG
    #define msg_name       name
    #define msg_namelen    namelen
    #define msg_iov        lpBuffers
    #define msg_iovlen     dwBufferCount
    #define msg_control    Control.buf
    #define msg_controllen Control.len
    #define msg_flags      flags

    // Windows doesn't have this flag, maybe we can implement it later for recvmsg
    #define MSG_DONTWAIT 0

// Reimplement the recvmsg function
int recvmsg(fd_t fd, msghdr* msg, int flags);

int sendmsg(fd_t fd, msghdr* msg, int flags);

/**
 * This struct is used to setup WSA on windows in a single place so we don't have to worry about it.
 *
 * A single instance of this struct will be created statically at startup which will ensure that WSA is setup for the
 * lifetime of the program and torn down as it exits.
 */
struct WSAHolder {
    static WSAHolder instance;
    WSAHolder();
    ~WSAHolder();
};

}  // namespace NUClear

#else

    // Include real networking stuff
    #include <arpa/inet.h>
    #include <fcntl.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <poll.h>
    #include <sys/ioctl.h>
    #include <sys/socket.h>
    #include <sys/types.h>
    #include <unistd.h>

    #include <cerrno>

    // Move errno so it can be used in windows
    #define network_errno  errno
    #define INVALID_SOCKET (-1)

namespace NUClear {
using fd_t = int;
}  // namespace NUClear

#endif

#endif  // NUCLEAR_UTIL_PLATFORM_HPP
