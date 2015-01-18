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

#ifndef NUCLEAR_UTIL_GET_NETWORK_INTERFACES_H
#define NUCLEAR_UTIL_GET_NETWORK_INTERFACES_H

#include <tuple>

#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>

namespace NUClear {
    namespace util {
        
        struct Interface {
            std::string name;
            uint32_t ip;
            uint32_t broadcast;
            uint16_t mtu;
        };
        
        inline std::vector<Interface> get_network_interfaces() {
            
            // We need a socket to query off
            int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            
            // Our ifconf queiry packet and supporting buffer
            char buf[16384];
            ifconf ifc;
            ifc.ifc_len=sizeof(buf);
            ifc.ifc_buf = buf;
            
            // Run our query
            if(ioctl(fd, SIOCGIFCONF, &ifc) < 0) {
                throw std::system_error(errno, std::system_category(), "Unable to run the interface query");
            }
            
            std::vector<Interface> ifaces;
            
            // Loop through our returned structures
            for(ifreq* r = ifc.ifc_req; r < ifc.ifc_req + (ifc.ifc_len / sizeof(ifreq));) {
                
                // Somewhat platform dependant as to if this variable exists
                #ifndef linux
                int len=IFNAMSIZ + r->ifr_addr.sa_len;
                #else
                int len=sizeof(ifreq);
                #endif
                
                // Make a new interface
                Interface iface;
                sockaddr_in* ip;
                
                // Store the name
                iface.name = r->ifr_name;
                
                // Get local address
                ioctl(fd, SIOCGIFADDR, r);
                ip = reinterpret_cast<sockaddr_in*>(&r->ifr_addr);
                iface.ip = ntohl(ip->sin_addr.s_addr);
                
                // Get our broadcast address
                ioctl(fd, SIOCGIFBRDADDR, r);
                ip = reinterpret_cast<sockaddr_in*>(&r->ifr_broadaddr);
                iface.broadcast = ntohl(ip->sin_addr.s_addr);
                
                // Get our MTU
                ioctl(fd, SIOCGIFMTU, r);
                iface.mtu = r->ifr_mtu;
                
                ifaces.push_back(iface);
                
                // Move along to the next object
                r = reinterpret_cast<ifreq*>((reinterpret_cast<char*>(r) + len));
            }
            
            // Remove duplicates from ifaces
            ifaces.erase(std::unique(std::begin(ifaces), std::end(ifaces), [] (const Interface& a, const Interface& b) {
                return a.name == b.name;
            }), std::end(ifaces));
            
            // Close our file descriptor
            close(fd);
            
            return ifaces;
        }
    }
}

#endif