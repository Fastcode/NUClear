/*
 * MIT License
 *
 * Copyright (c) 2016 NUClear Contributors
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

#ifdef _WIN32

    #include "platform.hpp"

    #include <stdexcept>
    #include <system_error>
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

    ::closesocket(sock);
}

namespace NUClear {
int recvmsg(fd_t fd, msghdr* msg, int /*flags*/) {

    // If we haven't setup our recvmsg function yet, set it up
    if (WSARecvMsg == nullptr) {
        initialize_WSARecvMsg();
    }

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

WSAHolder::WSAHolder() {
    WORD version = MAKEWORD(2, 2);
    WSADATA wsa_data;

    int startup_status = WSAStartup(version, &wsa_data);
    if (startup_status != 0) {
        throw std::system_error(startup_status, std::system_category(), "WSAStartup() failed");
    }
}

WSAHolder::~WSAHolder() {
    WSACleanup();
}

WSAHolder WSAHolder::instance{};

}  // namespace NUClear

#endif
