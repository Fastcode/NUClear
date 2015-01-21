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

#include <net/if.h>
#include <cstring>

#include "nuclear_bits/PowerPlant.h"
#include "nuclear_bits/dsl/word/IO.h"
#include "nuclear_bits/util/generate_reaction.h"
#include "nuclear_bits/util/get_network_interfaces.h"

namespace NUClear {
    namespace dsl {
        namespace word {
            
            struct UDP {
                
                struct Packet {
                    /// If there was an error while reading the packet
                    bool error;
                    /// The address that the packet is from/to
                    uint32_t address;
                    /// The data to be sent in the packet
                    std::vector<char> data;
                };
                
                template <typename DSL, typename TFunc>
                static inline threading::ReactionHandle bind(Reactor& reactor, const std::string& label, TFunc&& callback, int port) {
                    
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
                    address.sin_addr.s_addr = htonl(INADDR_ANY);
                    
                    // Bind to the address, and if we fail throw an error
                    if(::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                        throw std::system_error(errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                    }
                    
                    // Generate a reaction for the IO system that closes on death
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
                    return handle;
                }
                
                template <typename DSL>
                static inline Packet get(threading::ReactionTask&) {
                    
                    // Get our filedescriptor from the magic cache
                    int fd = store::ThreadStore<int, 0>::value;
                    
                    // Make a packet with 2k of storage (hopefully packets are smaller then this as most MTUs are around 1500)
                    Packet p;
                    p.error = true;
                    p.data.resize(2048);
                    
                    // Make a socket address to store our sender information
                    sockaddr_in from;
                    socklen_t sSize = sizeof(sockaddr_in);
                    
                    ssize_t recieved = recvfrom(fd, p.data.data(), p.data.size(), 0, reinterpret_cast<sockaddr*>(&from), &sSize);
                    
                    // if no error
                    if(recieved > 0) {
                        p.error = false;
                        p.address = ntohl(from.sin_addr.s_addr);
                        p.data.resize(recieved);
                    }
                    
                    return p;
                }
                
                struct Broadcast {
                    
                    template <typename DSL, typename TFunc>
                    static inline threading::ReactionHandle bind(Reactor& reactor, const std::string& label, TFunc&& callback, int port) {
                       
                        // Our list of broadcast file descriptors
                        std::vector<int> fds;
                        
                        // Get all the network interfaces
                        auto interfaces = util::get_network_interfaces();
                        
                        std::vector<uint32_t> addresses;
                        
                        for(auto& iface : interfaces) {
                            // We don't care about interfaces with a netmask of 255.255.255.255
                            if(iface.ip != iface.broadcast) {
                                // Two broadcast ips that are the same are probably on the same network so ignore those
                                if(std::find(std::begin(addresses), std::end(addresses), iface.broadcast) == std::end(addresses)) {
                                    addresses.push_back(iface.broadcast);
                                }
                            }
                        }
                        
                        for(auto& ad : addresses) {
                            
                            // Make our socket
                            int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                            if(fd < 0) {
                                throw std::system_error(errno, std::system_category(), "We were unable to open the UDP socket");
                            }
                            
                            // The address we will be binding to (our broadcast address)
                            sockaddr_in address;
                            memset(&address, 0, sizeof(sockaddr_in));
                            address.sin_family = AF_INET;
                            address.sin_port = htons(port);
                            address.sin_addr.s_addr = htonl(ad);
                            
                            // We are a broadcast socket
                            int yes = true;
                            if(setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0) {
                                throw std::system_error(errno, std::system_category(), "We were unable to set the socket as broadcast");
                            }
                            if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
                                throw std::system_error(errno, std::system_category(), "We were unable to set reuse address on the socket");
                            }
                            
                            // Bind to the address, and if we fail throw an error
                            if(::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(sockaddr))) {
                                throw std::system_error(errno, std::system_category(), "We were unable to bind the UDP socket to the port");
                            }
                            
                            fds.push_back(fd);
                        }
                        
                        // Generate a reaction for the IO system that closes on death
                        auto reaction = util::generate_reaction<DSL, IO>(reactor, label, std::forward<TFunc>(callback), [fds] {
                            // Close all the sockets
                            for(auto& fd : fds) {
                                ::close(fd);
                            }
                        });
                        std::shared_ptr<threading::Reaction> r(std::move(reaction));
                        threading::ReactionHandle handle(r.get());
                        
                        // Send our configuration out for each file descriptor (same reaction)
                        for(auto& fd : fds) {
                            reactor.powerplant.emit<emit::Direct>(std::make_unique<IOConfiguration>(IOConfiguration {
                                fd,
                                IO::READ,
                                r
                            }));
                        }
                        
                        // Return our handles
                        return handle;
                    }
                    
                    template <typename DSL>
                    static inline Packet get(threading::ReactionTask&) {
                        
                        // Get our filedescriptor from the magic cache
                        int fd = store::ThreadStore<int, 0>::value;
                        
                        // Make a packet with 2k of storage (hopefully packets are smaller then this as most MTUs are around 1500)
                        Packet p;
                        p.error = true;
                        p.data.resize(2048);
                        
                        // Make a socket address to store our sender information
                        sockaddr_in from;
                        socklen_t sSize = sizeof(sockaddr_in);
                        
                        ssize_t recieved = recvfrom(fd, p.data.data(), p.data.size(), 0, reinterpret_cast<sockaddr*>(&from), &sSize);
                        
                        // if no error
                        if(recieved > 0) {
                            p.error = false;
                            p.address = ntohl(from.sin_addr.s_addr);
                            p.data.resize(recieved);
                        }
                        
                        return p;
                    }

                };
            };
        }
    }
}

#endif
