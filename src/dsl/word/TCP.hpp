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

#ifndef NUCLEAR_DSL_WORD_TCP_HPP
#define NUCLEAR_DSL_WORD_TCP_HPP

#include <cstring>

#include "../../PowerPlant.hpp"
#include "../../util/FileDescriptor.hpp"
#include "../../util/platform.hpp"
#include "IO.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This allows a reaction to be triggered based on TCP activity.
         *
         * @details
         *  @code on<TCP>(port) @endcode
         *  When a connection is identified on the assigned port, the associated reaction will be triggered.  The
         *  request for a TCP based reaction can use a runtime argument to reference a specific port.  Note that the
         *  port reference can be changed during the systems execution phase.
         *
         *  @code on<TCP>() @endcode
         *  Should the port reference be omitted, then the system will bind to a currently unassigned port.
         *
         *  @code on<TCP, TCP>(port, port)  @endcode
         *  A reaction can also be triggered via connectivity on more than one port.
         *
         * @attention
         *  Because TCP communications are stream based, the on< TCP >() request will often require an on< IO >()
         *  request also be specified within its definition. It is the later request which will define the reaction to
         *  run when activity on the stream is detected.  For example:
         *  @code on<TCP>(port).then([this](const TCP::Connection& connection){
         *    on<IO>(connection.fd, IO::READ | IO::CLOSE).then([this](IO::Event event)
         *  } @endcode
         *
         * @par Implements
         *  Bind
         */
        struct TCP {

            struct Connection {

                struct {
                    uint32_t address;
                    uint16_t port;
                } remote;

                struct {
                    uint32_t address;
                    uint16_t port;
                } local;

                fd_t fd;

                operator bool() const {
                    return fd != 0;
                }
            };

            template <typename DSL>
            static inline std::tuple<in_port_t, fd_t> bind(const std::shared_ptr<threading::Reaction>& reaction,
                                                           in_port_t port = 0) {

                // Make our socket
                util::FileDescriptor fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                if (fd < 0) {
                    throw std::system_error(
                        network_errno, std::system_category(), "We were unable to open the TCP socket");
                }

                // The address we will be binding to
                sockaddr_in address;
                memset(&address, 0, sizeof(sockaddr_in));
                address.sin_family      = AF_INET;
                address.sin_port        = htons(port);
                address.sin_addr.s_addr = htonl(INADDR_ANY);

                // Bind to the address, and if we fail throw an error
                if (::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                    throw std::system_error(
                        network_errno, std::system_category(), "We were unable to bind the TCP socket to the port");
                }

                // Listen to the address
                if (::listen(fd, 1024) < 0) {
                    throw std::system_error(
                        network_errno, std::system_category(), "We were unable to listen on the TCP socket");
                }

                // Get the port we ended up listening on
                socklen_t len = sizeof(sockaddr_in);
                if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                    throw std::system_error(
                        network_errno, std::system_category(), "We were unable to get the port from the TCP socket");
                }
                port = ntohs(address.sin_port);

                // Generate a reaction for the IO system that closes on death
                fd_t cfd = fd;
                reaction->unbinders.push_back([](const threading::Reaction& r) {
                    r.reactor.emit<emit::Direct>(std::make_unique<operation::Unbind<IO>>(r.id));
                });
                reaction->unbinders.push_back([cfd](const threading::Reaction&) { close(cfd); });

                auto io_config = std::make_unique<IOConfiguration>(IOConfiguration{fd.release(), IO::READ, reaction});

                // Send our configuration out
                reaction->reactor.emit<emit::Direct>(io_config);

                // Return our handles
                return std::make_tuple(port, cfd);
            }

            template <typename DSL>
            static inline Connection get(threading::Reaction& r) {

                // Get our file descriptor from the magic cache
                auto event = IO::get<DSL>(r);

                // If our get is being run without an fd (something else triggered) then short circuit
                if (event.fd == 0) { return Connection{{0, 0}, {0, 0}, 0}; }
                else {
                    // Accept our connection
                    sockaddr_in local;
                    sockaddr_in remote;
                    socklen_t size = sizeof(sockaddr_in);

                    // Accept the remote connection
                    util::FileDescriptor fd = ::accept(event.fd, reinterpret_cast<sockaddr*>(&remote), &size);

                    // Get our local address
                    ::getsockname(fd, reinterpret_cast<sockaddr*>(&local), &size);

                    if (fd == -1) { return Connection{{0, 0}, {0, 0}, 0}; }
                    else {
                        return Connection{{ntohl(remote.sin_addr.s_addr), ntohs(remote.sin_port)},
                                          {ntohl(local.sin_addr.s_addr), ntohs(local.sin_port)},
                                          fd.release()};
                    }
                }
            }
        };

    }  // namespace word

    // Leaving this here as having it as transient is a big problem
    // It seems like it should be transient, however most reactions that use this will
    // get an unpleasant surprise if they are given the same "new connection" twice.
    namespace trait {

        template <>
        struct is_transient<word::TCP::Connection> : public std::false_type {};

    }  // namespace trait
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_TCP_HPP
