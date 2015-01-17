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

#ifndef NUCLEAR_DSL_WORD_UDP_H
#define NUCLEAR_DSL_WORD_UDP_H

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "nuclear_bits/PowerPlant.h"
#include "nuclear_bits/dsl/word/IO.h"
#include "nuclear_bits/util/generate_reaction.h"

namespace NUClear {
    namespace dsl {
        namespace word {
            
            struct UDP {
                
                struct Packet {
                    std::string from;
                    std::vector<char> data;
                };
                
                template <typename DSL, typename TFunc>
                static inline std::vector<threading::ReactionHandle> bind(Reactor& reactor, const std::string& label, TFunc&& callback, int port) {
                    
                    // Make our socket
                    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                    if(fd < 0) {
                        throw std::system_error(errno, std::system_category(), "We were unable to open the UDP socket");
                    }
                    
                    // The address we will be binding to
                    sockaddr_in address;
                    memset(&address, 0, sizeof(sockaddr_in));
                    address.sin_family = AF_INET;
                    address.sin_port = htons(port);
                    address.sin_addr.s_addr = INADDR_ANY;
                    
                    
                    if(::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                        throw std::system_error(errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                    }
                    
                    // Generate an IO reaction
                    auto reaction = util::generate_reaction<DSL, IO>(reactor, label, std::forward<TFunc>(callback), [fd] {
                        ::close(fd);
                    });
                    threading::ReactionHandle handle(reaction.get());
                    
                    // Send our configuration out
                    reactor.powerplant.emit<emit::Direct>(std::make_unique<IOConfiguration>(IOConfiguration {
                        fd,
                        IO::READ,
                        std::move(reaction)
                    }));
                    
                    // Return our handles
                    std::vector<threading::ReactionHandle> handles = { handle };
                    return handles;
                    
                    
                    
                }
                
                template <typename DSL>
                static inline Packet get(threading::ReactionTask&) {
                    
                    int fd = store::ThreadStore<int, 0>::value;
                    int interestSet = store::ThreadStore<int, 1>::value;
                    
//                    recv(socket, buffer, buffersize, 0);
                    
                    
                    // Read our UDP packet
                    
                    // Make it nice
                    
                    // Return it from the get
                    
                    // Return our thread local variable
                    return Packet();
                };
            };
        }
    }
}

#endif
