/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
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

#include "has_multicast.hpp"

#include <algorithm>

#include "util/network/get_interfaces.hpp"
#include "util/platform.hpp"

namespace test_util {

namespace {

/**
 * Attempt an actual multicast send/receive round-trip.
 * Returns true only if the packet is successfully delivered.
 * This detects environments (e.g., macOS CI VMs) where interfaces report IFF_MULTICAST
 * but the hypervisor doesn't actually deliver multicast packets.
 */
bool test_multicast_roundtrip(int af, const char* group_addr) {
    // Create a UDP socket for receiving
    NUClear::fd_t recv_fd = ::socket(af, SOCK_DGRAM, 0);
    if (recv_fd < 0) {
        return false;
    }

    // Allow address reuse
    int one = 1;
    ::setsockopt(recv_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&one), sizeof(one));
#ifdef SO_REUSEPORT
    ::setsockopt(recv_fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char*>(&one), sizeof(one));
#endif

    // Bind to any address on an ephemeral port
    uint16_t port = 0;
    if (af == AF_INET) {
        sockaddr_in bind_addr{};
        bind_addr.sin_family      = AF_INET;
        bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        bind_addr.sin_port        = 0;

        if (::bind(recv_fd, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
            ::close(recv_fd);
            return false;
        }

        // Get the assigned port
        socklen_t len = sizeof(bind_addr);
        ::getsockname(recv_fd, reinterpret_cast<sockaddr*>(&bind_addr), &len);
        port = ntohs(bind_addr.sin_port);

        // Join the multicast group
        struct ip_mreq mreq {};
        ::inet_pton(AF_INET, group_addr, &mreq.imr_multiaddr);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (::setsockopt(recv_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&mreq), sizeof(mreq))
            < 0) {
            ::close(recv_fd);
            return false;
        }
    }
    else {
        sockaddr_in6 bind_addr{};
        bind_addr.sin6_family = AF_INET6;
        bind_addr.sin6_addr   = in6addr_any;
        bind_addr.sin6_port   = 0;

        if (::bind(recv_fd, reinterpret_cast<sockaddr*>(&bind_addr), sizeof(bind_addr)) < 0) {
            ::close(recv_fd);
            return false;
        }

        socklen_t len = sizeof(bind_addr);
        ::getsockname(recv_fd, reinterpret_cast<sockaddr*>(&bind_addr), &len);
        port = ntohs(bind_addr.sin6_port);

        // Join the multicast group
        struct ipv6_mreq mreq {};
        ::inet_pton(AF_INET6, group_addr, &mreq.ipv6mr_multiaddr);
        mreq.ipv6mr_interface = 0;
        if (::setsockopt(recv_fd,
                         IPPROTO_IPV6,
                         IPV6_JOIN_GROUP,
                         reinterpret_cast<const char*>(&mreq),
                         sizeof(mreq))
            < 0) {
            ::close(recv_fd);
            return false;
        }
    }

    // Create a send socket
    NUClear::fd_t send_fd = ::socket(af, SOCK_DGRAM, 0);
    if (send_fd < 0) {
        ::close(recv_fd);
        return false;
    }

    // Set multicast loopback so we receive our own packet
    if (af == AF_INET) {
        uint8_t loop = 1;
        ::setsockopt(send_fd, IPPROTO_IP, IP_MULTICAST_LOOP, reinterpret_cast<const char*>(&loop), sizeof(loop));
    }
    else {
        int loop = 1;
        ::setsockopt(send_fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, reinterpret_cast<const char*>(&loop), sizeof(loop));
    }

    // Send a test packet to the multicast group
    const char test_msg[] = "MCAST_TEST";
    if (af == AF_INET) {
        sockaddr_in dest{};
        dest.sin_family = AF_INET;
        dest.sin_port   = htons(port);
        ::inet_pton(AF_INET, group_addr, &dest.sin_addr);
        ::sendto(send_fd,
                 test_msg,
                 sizeof(test_msg),
                 0,
                 reinterpret_cast<sockaddr*>(&dest),
                 sizeof(dest));
    }
    else {
        sockaddr_in6 dest{};
        dest.sin6_family = AF_INET6;
        dest.sin6_port   = htons(port);
        ::inet_pton(AF_INET6, group_addr, &dest.sin6_addr);
        ::sendto(send_fd,
                 test_msg,
                 sizeof(test_msg),
                 0,
                 reinterpret_cast<sockaddr*>(&dest),
                 sizeof(dest));
    }

    // Wait for the packet with a 200ms timeout using select (portable across all platforms)
    fd_set read_fds;
    FD_ZERO(&read_fds);    // NOLINT(readability-isolate-declaration)
    FD_SET(recv_fd, &read_fds);  // NOLINT(hicpp-signed-bitwise)
    struct timeval tv {};
    tv.tv_sec  = 0;
    tv.tv_usec = 200000;  // 200ms

    int ready = ::select(static_cast<int>(recv_fd) + 1, &read_fds, nullptr, nullptr, &tv);

    ::close(send_fd);
    ::close(recv_fd);

    return ready > 0;
}

}  // namespace

bool has_ipv4_multicast() {
    // First check if any interface reports multicast support
    auto ifaces = NUClear::util::network::get_interfaces();
    bool has_flag = std::any_of(ifaces.begin(), ifaces.end(), [](const auto& iface) {
        return iface.ip.sock.sa_family == AF_INET && iface.flags.multicast;
    });
    if (!has_flag) {
        return false;
    }

    // Then verify multicast actually works with a real round-trip
    return test_multicast_roundtrip(AF_INET, "239.255.255.250");
}

bool has_ipv6_multicast() {
    // First check if any interface reports multicast support
    auto ifaces = NUClear::util::network::get_interfaces();
    bool has_flag = std::any_of(ifaces.begin(), ifaces.end(), [](const auto& iface) {
        return iface.ip.sock.sa_family == AF_INET6 && iface.flags.multicast;
    });
    if (!has_flag) {
        return false;
    }

    // Then verify multicast actually works with a real round-trip
    return test_multicast_roundtrip(AF_INET6, "ff02::1");
}

}  // namespace test_util
