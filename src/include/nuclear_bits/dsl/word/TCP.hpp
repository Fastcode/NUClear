/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_WORD_TCP_H
#define NUCLEAR_DSL_WORD_TCP_H

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <net/if.h>
#include <cstring>

#include "nuclear_bits/PowerPlant.hpp"
#include "nuclear_bits/dsl/word/IO.hpp"
#include "nuclear_bits/util/generate_reaction.hpp"
#include "nuclear_bits/util/FileDescriptor.hpp"


namespace NUClear {
    namespace dsl {
        namespace word {

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

                    int fd;

                    operator bool() const {
                        return fd != 0;
                    }
                };

                template <typename DSL, typename TFunc>
                static inline std::tuple<threading::ReactionHandle, int, int> bind(Reactor& reactor, const std::string& label, TFunc&& callback, int port = 0) {

                    // Make our socket
                    util::FileDescriptor fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

                    if(fd < 0) {
                        throw std::system_error(errno, std::system_category(), "We were unable to open the TCP socket");
                    }

                    // The address we will be binding to
                    sockaddr_in address;
                    memset(&address, 0, sizeof(sockaddr_in));
                    address.sin_family = AF_INET;
                    address.sin_port = htons(port);
                    address.sin_addr.s_addr = htonl(INADDR_ANY);

                    // Bind to the address, and if we fail throw an error
                    if(::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                        throw std::system_error(errno, std::system_category(), "We were unable to bind the TCP socket to the port");
                    }

                    // Listen to the address
                    if(::listen(fd, 1024) < 0) {
                        throw std::system_error(errno, std::system_category(), "We were unable to listen on the TCP socket");
                    }

                    // Get the port we ended up listening on
                    socklen_t len = sizeof(sockaddr_in);
                    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&address), &len) == -1) {
                        throw std::system_error(errno, std::system_category(), "We were unable to get the port from the TCP socket");
                    }
                    port = ntohs(address.sin_port);

                    // Generate a reaction for the IO system that closes on death
                    int cfd = fd;
                    auto reaction = util::generate_reaction<DSL, IO>(reactor, label, std::forward<TFunc>(callback), [cfd] (threading::Reaction&) {
                        ::close(cfd);
                    });

                    auto ioConfig = std::make_unique<IOConfiguration>(IOConfiguration {
                        fd.release(),
                        IO::READ,
                        std::move(reaction)
                    });

                    threading::ReactionHandle handle(ioConfig->reaction);

                    // Send our configuration out
                    reactor.powerplant.emit<emit::Direct>(ioConfig);

                    // Return our handles
                    return std::make_tuple(handle, port, cfd);
                }

                template <typename DSL>
                static inline Connection get(threading::Reaction& r) {

                    // Get our file descriptor from the magic cache
                    auto event = IO::get<DSL>(r);

                    // If our get is being run without an fd (something else triggered) then short circuit
                    if (event.fd == 0) {
                        return Connection { { 0, 0 }, { 0, 0 }, 0 };
                    }
                    else {
                        // Accept our connection
                        sockaddr_in local;
                        sockaddr_in remote;
                        socklen_t size = sizeof(sockaddr_in);

                        // Accept the remote connection
                        util::FileDescriptor fd = ::accept(event.fd, reinterpret_cast<sockaddr*>(&remote), &size);

                        // Get our local address
                        ::getsockname(fd, reinterpret_cast<sockaddr*>(&local), &size);

                        if (fd < 0) {
                            return Connection { { 0, 0 }, { 0, 0 }, 0 };
                        }
                        else {
                            return Connection { { ntohl(remote.sin_addr.s_addr), ntohs(remote.sin_port) }, { ntohl(local.sin_addr.s_addr), ntohs(local.sin_port)} , fd.release() };
                        }
                    }
                }
            };
        }

        namespace trait {
            template <>
            struct is_transient<word::TCP::Connection> : public std::true_type {};
        }
    }
}

#endif
