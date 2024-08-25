/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#include "util/network/resolve.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>


TEST_CASE("resolve function returns expected socket address", "[util][network][resolve]") {

    SECTION("IPv4 address") {
        const std::string address = "127.0.0.1";
        const uint16_t port       = 80;

        const auto result = NUClear::util::network::resolve(address, port);

        REQUIRE(result.sock.sa_family == AF_INET);
        REQUIRE(ntohs(result.ipv4.sin_port) == port);
        REQUIRE(ntohl(result.ipv4.sin_addr.s_addr) == INADDR_LOOPBACK);
    }

    SECTION("IPv6 address") {
        const std::string address = "::1";
        const uint16_t port       = 80;

        const auto result = NUClear::util::network::resolve(address, port);

        REQUIRE(result.sock.sa_family == AF_INET6);
        REQUIRE(ntohs(result.ipv6.sin6_port) == port);
        // Check each byte of the address except the last one is 0
        for (int i = 0; i < 15; i++) {
            REQUIRE(result.ipv6.sin6_addr.s6_addr[i] == 0);
        }
        // Last byte should be 1
        REQUIRE(result.ipv6.sin6_addr.s6_addr[15] == 1);
    }

    SECTION("Hostname") {
        const std::string address = "localhost";
        const uint16_t port       = 80;

        const auto result = NUClear::util::network::resolve(address, port);

        // Check that the returned socket address matches the expected address and port
        REQUIRE((result.sock.sa_family == AF_INET || result.sock.sa_family == AF_INET6));

        // Localhost could return an ipv4 or ipv6 address
        if (result.sock.sa_family == AF_INET) {
            REQUIRE(ntohs(result.ipv4.sin_port) == port);
            REQUIRE(ntohl(result.ipv4.sin_addr.s_addr) == INADDR_LOOPBACK);
        }
        else {
            REQUIRE(ntohs(result.ipv6.sin6_port) == port);
            // Check each byte of the address except the last one is 0
            for (int i = 0; i < 15; i++) {
                REQUIRE(result.ipv6.sin6_addr.s6_addr[i] == 0);
            }
            // Last byte should be 1
            REQUIRE(result.ipv6.sin6_addr.s6_addr[15] == 1);
        }
    }

    SECTION("IPv6 address with mixed case letters") {
        const std::string address = "2001:0DB8:Ac10:FE01:0000:0000:0000:0000";
        const uint16_t port       = 80;

        const auto result = NUClear::util::network::resolve(address, port);

        REQUIRE(result.sock.sa_family == AF_INET6);
        REQUIRE(ntohs(result.ipv6.sin6_port) == port);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[0] == 0x20);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[1] == 0x01);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[2] == 0x0d);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[3] == 0xb8);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[4] == 0xac);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[5] == 0x10);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[6] == 0xfe);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[7] == 0x01);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[8] == 0x00);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[9] == 0x00);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[10] == 0x00);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[11] == 0x00);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[12] == 0x00);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[13] == 0x00);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[14] == 0x00);
        REQUIRE(result.ipv6.sin6_addr.s6_addr[15] == 0x00);
    }

    SECTION("Hostname with valid IPv4 address") {
        const std::string address = "ipv4.google.com";
        const uint16_t port       = 80;

        const auto result = NUClear::util::network::resolve(address, port);

        REQUIRE(result.sock.sa_family == AF_INET);
        REQUIRE(ntohs(result.ipv4.sin_port) == port);
        REQUIRE(ntohl(result.ipv4.sin_addr.s_addr) != 0);
    }

// For some reason windows hates this test and I'm not sure what to check to see if a windows instance can do this
#ifndef _WIN32
    SECTION("Hostname with valid IPv6 address") {
        const std::string address = "ipv6.google.com";
        const uint16_t port       = 80;

        const auto result = NUClear::util::network::resolve(address, port);

        REQUIRE(result.sock.sa_family == AF_INET6);
        REQUIRE(ntohs(result.ipv6.sin6_port) == port);

        // Check if all components are zero
        bool nonzero = false;
        for (int i = 0; i < 15; i++) {
            nonzero |= result.ipv6.sin6_addr.s6_addr[i] != 0;
        }

        REQUIRE(nonzero);
    }
#endif

    SECTION("Invalid address") {
        const std::string address = "this.url.is.invalid";
        const uint16_t port       = 12345;

        // Check that the function throws a std::runtime_error with the appropriate message
        REQUIRE_THROWS(NUClear::util::network::resolve(address, port));
    }
}
