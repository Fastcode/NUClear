#include "util/network/resolve.hpp"

#include <catch.hpp>

TEST_CASE("resolve function returns expected socket address", "[util][network][resolve]") {

    SECTION("IPv4 address") {
        std::string address = "127.0.0.1";
        uint16_t port       = 80;

        auto result = NUClear::util::network::resolve(address, port);

        REQUIRE(result.sock.sa_family == AF_INET);
        REQUIRE(ntohs(result.ipv4.sin_port) == port);
        REQUIRE(ntohl(result.ipv4.sin_addr.s_addr) == INADDR_LOOPBACK);
    }

    SECTION("IPv6 address") {
        std::string address = "::1";
        uint16_t port       = 80;

        auto result = NUClear::util::network::resolve(address, port);

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
        std::string address = "localhost";
        uint16_t port       = 80;

        auto result = NUClear::util::network::resolve(address, port);

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

    SECTION("IPv4 address with leading zeros") {
        std::string address = "127.000.000.001";
        uint16_t port       = 80;

        auto result = NUClear::util::network::resolve(address, port);

        REQUIRE(result.sock.sa_family == AF_INET);
        REQUIRE(ntohs(result.ipv4.sin_port) == port);
        REQUIRE(ntohl(result.ipv4.sin_addr.s_addr) == INADDR_LOOPBACK);
    }

    SECTION("IPv6 address with mixed case letters") {
        std::string address = "2001:0DB8:Ac10:FE01:0000:0000:0000:0000";
        uint16_t port       = 80;

        auto result = NUClear::util::network::resolve(address, port);

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
        std::string address = "ipv4.google.com";
        uint16_t port       = 80;

        auto result = NUClear::util::network::resolve(address, port);

        REQUIRE(result.sock.sa_family == AF_INET);
        REQUIRE(ntohs(result.ipv4.sin_port) == port);
        REQUIRE(ntohl(result.ipv4.sin_addr.s_addr) != 0);
    }

    SECTION("Hostname with valid IPv6 address") {
        std::string address = "ipv6.google.com";
        uint16_t port       = 80;

        auto result = NUClear::util::network::resolve(address, port);

        REQUIRE(result.sock.sa_family == AF_INET6);
        REQUIRE(ntohs(result.ipv6.sin6_port) == port);

        // Check if all components are zero
        bool nonzero = false;
        for (int i = 0; i < 15; i++) {
            nonzero |= result.ipv6.sin6_addr.s6_addr[i] != 0;
        }

        REQUIRE(nonzero);
    }

    SECTION("Invalid address") {
        std::string address = "notahost";
        uint16_t port       = 12345;

        // Check that the function throws a std::runtime_error with the appropriate message
        REQUIRE_THROWS(NUClear::util::network::resolve(address, port));
    }
}
