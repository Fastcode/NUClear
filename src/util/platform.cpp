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

#include "platform.hpp"

#ifdef _WIN32

#    include <stdexcept>
LPFN_WSARECVMSG WSARecvMsg = nullptr;

// Go get that WSARecvMsg function from stupid land
void initialize_WSARecvMsg() {
    GUID guid      = WSAID_WSARECVMSG;
    SOCKET sock    = INVALID_SOCKET;
    DWORD dw_bytes = 0;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (SOCKET_ERROR
        == WSAIoctl(sock,
                    SIO_GET_EXTENSION_FUNCTION_POINTER,
                    &guid,
                    sizeof(guid),
                    &WSARecvMsg,
                    sizeof(WSARecvMsg),
                    &dw_bytes,
                    nullptr,
                    nullptr)) {
        throw std::runtime_error("We could not get WSARecvMsg from the OS");
    }

    closesocket(sock);
}

namespace NUClear {
int recvmsg(fd_t fd, msghdr* msg, int /* flags */) {

    // If we haven't setup our recvmsg function yet, set it up
    if (WSARecvMsg == nullptr) { initialize_WSARecvMsg(); }

    // Translate to windows speak
    DWORD received = 0;

    int v = WSARecvMsg(fd, msg, &received, nullptr, nullptr);

    return v == 0 ? received : v;
}

int sendmsg(fd_t fd, msghdr* msg, int flags) {

    DWORD sent = 0;
    auto v     = WSASendMsg(fd, msg, flags, &sent, nullptr, nullptr);

    return v == 0 ? sent : v;
}
}  // namespace NUClear

#endif
