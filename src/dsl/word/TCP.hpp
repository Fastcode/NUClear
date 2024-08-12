/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_TCP_HPP
#define NUCLEAR_DSL_WORD_TCP_HPP

#include <cstring>

#include "../../threading/Reaction.hpp"
#include "../../util/FileDescriptor.hpp"
#include "../../util/network/resolve.hpp"
#include "../../util/network/sock_t.hpp"
#include "../../util/platform.hpp"
#include "IO.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * This allows a reaction to be triggered based on TCP activity.
         *
         * @code on<TCP>(port) @endcode
         * When a connection is identified on the assigned port, the associated reaction will be triggered.
         * The request for a TCP based reaction can use a runtime argument to reference a specific port.
         * Note that the port reference can be changed during the systems execution phase.
         *
         * @code on<TCP>() @endcode
         * Should the port reference be omitted, then the system will bind to a currently unassigned port.
         *
         * @code on<TCP, TCP>(port, port)  @endcode
         * A reaction can also be triggered via connectivity on more than one port.
         *
         * @attention
         *  Because TCP communications are stream based, the on<TCP>() request will often require an on<IO>()
         *  request also be specified within its definition.
         *  It is the later request which will define the reaction to run when activity on the stream is detected.
         *  For example:
         *  @code on<TCP>(port).then([this](const TCP::Connection& connection){
         *    on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event)
         *  } @endcode
         *
         * @par Implements
         *  Bind
         */
        struct TCP : IO {

            struct Connection {

                struct Target {
                    /// The address of the connection
                    std::string address;
                    /// The port of the connection
                    uint16_t port;
                };

                /// The local address of the connection
                Target local;
                /// The remote address of the connection
                Target remote;

                /// The file descriptor for the connection
                fd_t fd;

                /**
                 * Casts this packet to a boolean to check if it is valid
                 *
                 * @return true if the packet is valid
                 */
                operator bool() const {
                    return fd != INVALID_SOCKET;
                }
            };

            template <typename DSL>
            static std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                    in_port_t port                  = 0,
                                                    const std::string& bind_address = "") {

                // Resolve the bind address if we have one
                util::network::sock_t address{};

                if (!bind_address.empty()) {
                    address = util::network::resolve(bind_address, port);
                }
                else {
                    address.ipv4.sin_family      = AF_INET;
                    address.ipv4.sin_port        = htons(port);
                    address.ipv4.sin_addr.s_addr = htonl(INADDR_ANY);
                }

                // Make our socket
                util::FileDescriptor fd(::socket(address.sock.sa_family, SOCK_STREAM, IPPROTO_TCP),
                                        [](const fd_t& f) { ::shutdown(f, SHUT_RDWR); });

                if (!fd.valid()) {
                    throw std::system_error(network_errno, std::system_category(), "Unable to open the TCP socket");
                }

                // Bind to the address, and if we fail throw an error
                if (::bind(fd, &address.sock, address.size())) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "Unable to bind the TCP socket to the port");
                }

                // Listen to the address
                if (::listen(fd, 1024) < 0) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "Unable to listen on the TCP socket");
                }

                // Get the port we ended up listening on
                socklen_t len = sizeof(address);
                if (::getsockname(fd, &address.sock, &len) == -1) {
                    throw std::system_error(network_errno,
                                            std::system_category(),
                                            "Unable to get the port from the TCP socket");
                }
                if (address.ipv4.sin_family == AF_INET6) {
                    port = ntohs(address.ipv6.sin6_port);
                }
                else {
                    port = ntohs(address.ipv4.sin_port);
                }

                // Generate a reaction for the IO system that closes on death
                const fd_t cfd = fd.release();

                reaction->unbinders.push_back([cfd](const threading::Reaction&) {
                    ::shutdown(cfd, SHUT_RDWR);
                    ::close(cfd);
                });

                // Bind using the IO system
                IO::bind<DSL>(reaction, cfd, IO::READ | IO::CLOSE);

                // Return our handles
                return std::make_tuple(port, cfd);
            }

            template <typename DSL>
            static Connection get(threading::ReactionTask& task) {

                // Get our file descriptor from the magic cache
                auto event = IO::get<DSL>(task);

                // If our get is being run without an fd (something else triggered) then short circuit
                if (!event) {
                    return {};
                }

                // Accept our connection
                util::network::sock_t local{};
                util::network::sock_t remote{};

                // Accept the remote connection
                socklen_t remote_size = sizeof(util::network::sock_t);
                util::FileDescriptor fd(::accept(event.fd, &remote.sock, &remote_size),
                                        [](const fd_t& f) { ::shutdown(f, SHUT_RDWR); });

                // Get our local address
                socklen_t local_size = sizeof(util::network::sock_t);
                ::getsockname(fd, &local.sock, &local_size);

                if (!fd.valid()) {
                    return Connection{};
                }

                auto local_s  = local.address();
                auto remote_s = remote.address();

                return Connection{{local_s.first, local_s.second}, {remote_s.first, remote_s.second}, fd.release()};
            }
        };

    }  // namespace word

    // Leaving this here as having it as transient is a big problem
    // It seems like it should be transient, however most reactions that use this will
    // get an unpleasant surprise if they are given the same "new connection" twice.
    namespace trait {

        template <>
        struct is_transient<word::TCP::Connection> : std::false_type {};

    }  // namespace trait
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_TCP_HPP
