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
#include "util/network/sock_t.hpp"

#include <netinet/in.h>
#include <sys/socket.h>

#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <tuple>

namespace NUClear {
namespace util {
    namespace network {

        SCENARIO("sock_t::size() returns the correct size for different address families", "[sock_t]") {
            GIVEN("An IPv4 address") {
                sock_t addr{};
                addr.sock.sa_family = AF_INET;

                THEN("its size should match sockaddr_in") {
                    REQUIRE(addr.size() == sizeof(sockaddr_in));
                }
            }

            GIVEN("An IPv6 address") {
                sock_t addr{};
                addr.sock.sa_family = AF_INET6;

                THEN("its size should match sockaddr_in6") {
                    REQUIRE(addr.size() == sizeof(sockaddr_in6));
                }
            }

            GIVEN("An unknown address family") {
                sock_t addr{};
                addr.sock.sa_family = AF_UNSPEC;  // Unknown family

                THEN("attempting to get its size should throw") {
                    REQUIRE_THROWS_AS(addr.size(), std::system_error);
                }
            }
        }

        SCENARIO("sock_t::address() resolves addresses correctly", "[sock_t]") {
            GIVEN("An IPv4 address") {
                sock_t addr{};
                addr.sock.sa_family       = AF_INET;
                addr.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1
                addr.ipv4.sin_port        = htons(12345);

                THEN("it should resolve to the correct IP and port") {
                    auto address_result = addr.address(true);  // Use numeric host
                    auto& host          = address_result.first;
                    auto& port          = address_result.second;
                    REQUIRE(host == "192.168.1.1");
                    REQUIRE(port == 12345);
                }
            }

            GIVEN("An IPv6 address") {
                sock_t addr{};
                addr.sock.sa_family = AF_INET6;
                // 2001:db8::1
                uint8_t ipv6_addr[16] =
                    {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
                std::memcpy(&addr.ipv6.sin6_addr, ipv6_addr, sizeof(ipv6_addr));
                addr.ipv6.sin6_port = htons(54321);

                THEN("it should resolve to the correct IP and port") {
                    auto address_result = addr.address(true);  // Use numeric host
                    auto& host          = address_result.first;
                    auto& port          = address_result.second;
                    REQUIRE(host == "2001:db8::1");
                    REQUIRE(port == 54321);
                }
            }

            GIVEN("An unsupported address family") {
                sock_t addr{};
                addr.sock.sa_family = AF_UNSPEC;  // Unsupported address family

                THEN("attempting to resolve it should throw") {
                    REQUIRE_THROWS_AS(addr.address(), std::system_error);
                }
            }
        }

        SCENARIO("sock_t equality operators (== and !=) correctly compare addresses", "[sock_t]") {
            GIVEN("Two identical IPv4 addresses") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET;
                addr2.sock.sa_family = AF_INET;

                addr1.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1
                addr2.ipv4.sin_addr.s_addr = htonl(0xC0A80101);

                addr1.ipv4.sin_port = htons(12345);
                addr2.ipv4.sin_port = htons(12345);

                THEN("they should be equal") {
                    REQUIRE(addr1 == addr2);
                    REQUIRE_FALSE(addr1 != addr2);
                }
            }

            GIVEN("Two IPv4 addresses with different IPs") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET;
                addr2.sock.sa_family = AF_INET;

                addr1.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1
                addr2.ipv4.sin_addr.s_addr = htonl(0xC0A80102);  // 192.168.1.2

                addr1.ipv4.sin_port = htons(12345);
                addr2.ipv4.sin_port = htons(12345);

                THEN("they should not be equal") {
                    REQUIRE_FALSE(addr1 == addr2);
                    REQUIRE(addr1 != addr2);
                }
            }

            GIVEN("Two IPv4 addresses with same IP but different ports") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET;
                addr2.sock.sa_family = AF_INET;

                addr1.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1
                addr2.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1

                addr1.ipv4.sin_port = htons(12345);
                addr2.ipv4.sin_port = htons(54321);

                THEN("they should not be equal") {
                    REQUIRE_FALSE(addr1 == addr2);
                    REQUIRE(addr1 != addr2);
                }
            }

            GIVEN("Two identical IPv6 addresses") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET6;
                addr2.sock.sa_family = AF_INET6;

                // 2001:db8::1
                uint8_t ipv6_addr[16] =
                    {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
                std::memcpy(&addr1.ipv6.sin6_addr, ipv6_addr, sizeof(ipv6_addr));
                std::memcpy(&addr2.ipv6.sin6_addr, ipv6_addr, sizeof(ipv6_addr));

                addr1.ipv6.sin6_port = htons(54321);
                addr2.ipv6.sin6_port = htons(54321);

                THEN("they should be equal") {
                    REQUIRE(addr1 == addr2);
                    REQUIRE_FALSE(addr1 != addr2);
                }
            }

            GIVEN("Two IPv6 addresses with different IPs") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET6;
                addr2.sock.sa_family = AF_INET6;

                // 2001:db8::1
                uint8_t ipv6_addr1[16] =
                    {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
                // 2001:db8::2
                uint8_t ipv6_addr2[16] =
                    {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
                std::memcpy(&addr1.ipv6.sin6_addr, ipv6_addr1, sizeof(ipv6_addr1));
                std::memcpy(&addr2.ipv6.sin6_addr, ipv6_addr2, sizeof(ipv6_addr2));

                addr1.ipv6.sin6_port = htons(54321);
                addr2.ipv6.sin6_port = htons(54321);

                THEN("they should not be equal") {
                    REQUIRE_FALSE(addr1 == addr2);
                    REQUIRE(addr1 != addr2);
                }
            }

            GIVEN("Two IPv6 addresses with same IP but different ports") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET6;
                addr2.sock.sa_family = AF_INET6;

                // 2001:db8::1
                uint8_t ipv6_addr[16] =
                    {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
                std::memcpy(&addr1.ipv6.sin6_addr, ipv6_addr, sizeof(ipv6_addr));
                std::memcpy(&addr2.ipv6.sin6_addr, ipv6_addr, sizeof(ipv6_addr));

                addr1.ipv6.sin6_port = htons(12345);
                addr2.ipv6.sin6_port = htons(54321);

                THEN("they should not be equal") {
                    REQUIRE_FALSE(addr1 == addr2);
                    REQUIRE(addr1 != addr2);
                }
            }

            GIVEN("An IPv4 and an IPv6 address") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET;
                addr2.sock.sa_family = AF_INET6;

                THEN("they should not be equal") {
                    REQUIRE_FALSE(addr1 == addr2);
                    REQUIRE(addr1 != addr2);
                }
            }

            GIVEN("A socket with an unsupported address family compared with a valid socket") {
                sock_t invalid_addr{};
                invalid_addr.sock.sa_family = AF_UNSPEC;

                sock_t valid_addr{};
                valid_addr.sock.sa_family       = AF_INET;
                valid_addr.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1
                valid_addr.ipv4.sin_port        = htons(12345);

                THEN("equality comparison should throw") {
                    REQUIRE_THROWS_AS(invalid_addr == valid_addr, std::system_error);
                    REQUIRE_THROWS_AS(invalid_addr != valid_addr, std::system_error);
                }
            }
        }

        SCENARIO("sock_t ordering operator (<) correctly orders addresses", "[sock_t]") {
            GIVEN("Two IPv4 addresses with different IPs") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET;
                addr2.sock.sa_family = AF_INET;

                addr1.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1
                addr2.ipv4.sin_addr.s_addr = htonl(0xC0A80102);  // 192.168.1.2

                addr1.ipv4.sin_port = htons(12345);
                addr2.ipv4.sin_port = htons(12345);

                THEN("addr1 should be less than addr2") {
                    REQUIRE(addr1 < addr2);
                }
            }

            GIVEN("Two IPv4 addresses with same IP but different ports") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET;
                addr2.sock.sa_family = AF_INET;

                addr1.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1
                addr2.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1

                addr1.ipv4.sin_port = htons(12345);
                addr2.ipv4.sin_port = htons(54321);

                THEN("addr1 should be less than addr2") {
                    REQUIRE(addr1 < addr2);
                }
            }

            GIVEN("Two IPv6 addresses with different IPs") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET6;
                addr2.sock.sa_family = AF_INET6;

                // 2001:db8::1
                uint8_t ipv6_addr1[16] =
                    {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
                // 2001:db8::2
                uint8_t ipv6_addr2[16] =
                    {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02};
                std::memcpy(&addr1.ipv6.sin6_addr, ipv6_addr1, sizeof(ipv6_addr1));
                std::memcpy(&addr2.ipv6.sin6_addr, ipv6_addr2, sizeof(ipv6_addr2));

                addr1.ipv6.sin6_port = htons(54321);
                addr2.ipv6.sin6_port = htons(54321);

                THEN("addr1 should be less than addr2") {
                    REQUIRE(addr1 < addr2);
                }
            }

            GIVEN("Two IPv6 addresses with same IP but different ports") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET6;
                addr2.sock.sa_family = AF_INET6;

                // 2001:db8::1
                uint8_t ipv6_addr[16] =
                    {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
                std::memcpy(&addr1.ipv6.sin6_addr, ipv6_addr, sizeof(ipv6_addr));
                std::memcpy(&addr2.ipv6.sin6_addr, ipv6_addr, sizeof(ipv6_addr));

                addr1.ipv6.sin6_port = htons(12345);
                addr2.ipv6.sin6_port = htons(54321);

                THEN("addr1 should be less than addr2") {
                    REQUIRE(addr1 < addr2);
                }
            }

            GIVEN("An IPv4 and an IPv6 address") {
                sock_t addr1{};
                sock_t addr2{};

                addr1.sock.sa_family = AF_INET;
                addr2.sock.sa_family = AF_INET6;

                THEN("IPv4 should be less than IPv6") {
                    REQUIRE(addr1 < addr2);
                }
            }

            GIVEN("A socket with an unsupported address family compared with a valid socket") {
                sock_t invalid_addr{};
                invalid_addr.sock.sa_family = AF_UNSPEC;

                sock_t valid_addr{};
                valid_addr.sock.sa_family       = AF_INET;
                valid_addr.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1
                valid_addr.ipv4.sin_port        = htons(12345);

                THEN("less than comparison should throw") {
                    REQUIRE_THROWS_AS(invalid_addr < valid_addr, std::system_error);
                }
            }
        }

        SCENARIO("sock_t stream operator (<<) correctly formats addresses", "[sock_t]") {
            GIVEN("An IPv4 address") {
                sock_t addr{};
                addr.sock.sa_family       = AF_INET;
                addr.ipv4.sin_addr.s_addr = htonl(0xC0A80101);  // 192.168.1.1
                addr.ipv4.sin_port        = htons(12345);

                THEN("it should stream in the correct format") {
                    std::stringstream ss;
                    ss << addr;
                    REQUIRE(ss.str() == "192.168.1.1:12345");
                }
            }

            GIVEN("An IPv6 address") {
                sock_t addr{};
                addr.sock.sa_family = AF_INET6;
                // 2001:db8::1
                uint8_t ipv6_addr[16] =
                    {0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
                std::memcpy(&addr.ipv6.sin6_addr, ipv6_addr, sizeof(ipv6_addr));
                addr.ipv6.sin6_port = htons(54321);

                THEN("it should stream in the correct format") {
                    std::stringstream ss;
                    ss << addr;
                    REQUIRE(ss.str() == "2001:db8::1:54321");
                }
            }

            GIVEN("An unsupported address family") {
                sock_t addr{};
                addr.sock.sa_family = AF_UNSPEC;  // Unsupported address family

                THEN("streaming should throw") {
                    std::stringstream ss;
                    REQUIRE_THROWS_AS(ss << addr, std::system_error);
                }
            }
        }

    }  // namespace network
}  // namespace util
}  // namespace NUClear
