/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP
#define NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP

#include "nuclear"
#include "nuclear_bits/extension/network/wire_protocol.hpp"

namespace NUClear {
namespace extension {

    class NetworkController : public Reactor {
    private:
        struct NetworkTarget {

            NetworkTarget(std::string name, in_addr_t address, in_port_t tcp_port, in_port_t udp_port, int tcp_fd)
                : name(name)
                , address(address)
                , tcp_port(tcp_port)
                , udp_port(udp_port)
                , tcp_fd(tcp_fd)
                , handle()
                , buffer()
                , buffer_mutex() {}

            std::string name;
            in_addr_t address;
            in_port_t tcp_port;
            in_port_t udp_port;
            fd_t tcp_fd;
            ReactionHandle handle;
            std::map<in_port_t, std::pair<clock::time_point, std::vector<std::vector<char>>>> buffer;
            std::mutex buffer_mutex;
        };

    public:
        explicit NetworkController(std::unique_ptr<NUClear::Environment> environment);

    private:
        void tcp_connection(const TCP::Connection& con);
        void tcp_handler(const IO::Event& event);
        void udp_handler(const UDP::Packet& packet);
        void tcp_send(const dsl::word::emit::NetworkEmit& emit);
        void udp_send(const dsl::word::emit::NetworkEmit& emit);
        void announce();

        // Our max UDP size is based of a 1500 MTU
        // Subtract the IP and UPD headers, and our protocol header size
        static constexpr const size_t MAX_UDP_PAYLOAD_LENGTH =
            1500 /*MTU*/ - 20 /*IP header*/ - 8 /*UDP header*/ - sizeof(network::DataPacket) + 1 /*Last char*/;
        static constexpr const size_t MAX_NUM_UDP_ASSEMBLY = 5;

        std::mutex write_mutex;

        ReactionHandle udp_handle;
        ReactionHandle tcp_handle;
        ReactionHandle multicast_handle;
        ReactionHandle multicast_emit_handle;
        ReactionHandle network_emit_handle;

        std::string name;
        std::string multicast_group;
        in_port_t multicast_port;
        in_port_t udp_port;
        in_port_t tcp_port;

        int udp_server_fd;
        int tcp_server_fd;

        std::atomic<uint16_t> packet_id_source;

        std::mutex reaction_mutex;
        std::multimap<std::array<uint64_t, 2>, std::shared_ptr<threading::Reaction>> reactions;

        std::list<NetworkTarget> targets;
        std::multimap<std::string, std::list<NetworkTarget>::iterator> name_target;
        std::map<std::pair<int, int>, std::list<NetworkTarget>::iterator> udp_target;
        std::map<int, std::list<NetworkTarget>::iterator> tcp_target;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP
