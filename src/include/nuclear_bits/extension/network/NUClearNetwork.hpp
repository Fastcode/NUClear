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

#ifndef NUCLEAR_EXTENSION_NETWORK_NUCLEARNETWORK_HPP
#define NUCLEAR_EXTENSION_NETWORK_NUCLEARNETWORK_HPP

#include <arpa/inet.h>
#include <array>
#include <atomic>
#include <functional>
#include <list>
#include <map>
#include <vector>
#include "wire_protocol.hpp"

namespace NUClear {
namespace extension {
    namespace network {

        class NUClearNetwork {
        public:
            struct NetworkTarget {

                NetworkTarget(std::string name,
                              sockaddr target,
                              std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now())
                    : name(name), target(target), last_update(last_update), assembly_mutex(), assembly() {}

                /// The name of the remote target
                std::string name;
                /// The socket address for the remote target
                sockaddr target;
                /// When we last received data from the remote target
                std::chrono::steady_clock::time_point last_update;
                /// Mutex to protect the fragmented packet storage
                std::mutex assembly_mutex;
                /// Storage for fragmented packets while we build them
                std::map<uint16_t,
                         std::pair<std::chrono::steady_clock::time_point, std::map<uint16_t, std::vector<char>>>>
                    assembly;
            };

            explicit NUClearNetwork();
            ~NUClearNetwork();

            /**
             * @brief Send data using the NUClear network
             *
             * @param hash          the identifying hash for the data
             * @param data          the bytes that are to be sent
             * @param target        who we are sending to (blank means everyone)
             * @param reliable      if the delivery of the data should be ensured
             */
            void send(const std::array<uint64_t, 2>& hash,
                      const std::vector<char>& payload,
                      const std::string& target,
                      bool reliable);

            /**
             * @brief Set the callback to use when a data packet is completed
             *
             * @param f the callback function
             */
            void set_packet_callback(
                std::function<void(const NetworkTarget&, const std::array<uint64_t, 2>&, std::vector<char>&&)> f);

            /**
             * @brief Set the callback to use when a node joins the network
             *
             * @param f the callback function
             */
            void set_join_callback(std::function<void(std::string, sockaddr)> f);

            /**
             * @brief Set the callback to use when a node leaves the network
             *
             * @param f the callback function
             */
            void set_leave_callback(std::function<void(std::string, sockaddr)> f);

            /**
             * @brief Leave the NUClear network
             */
            void shutdown();

            /**
             * @brief Reset our network to use the new settings
             *
             * @details
             *  Resets the networking system to use the new mutlicast information and name.
             *  If the network was already joined, it will first leave and then rejoin.
             *
             * @param name  the name of this node in the network
             * @param group the multicast group to announce on
             * @param port  the multicast port to use
             */
            void reset(std::string name, std::string group, in_port_t port);

            /**
             * @brief Process waiting data in the UDP sockets and send them to the callback if they are relevant.
             */
            void process();

            /**
             * @brief Send an announce packet over UDP multicast
             */
            void announce();

            /**
             * @brief Get the file descriptors that the network listens on
             *
             * @return a list of file descriptors that the system listens on
             */
            std::vector<int> listen_fds();

        private:
            /**
             * @brief Open our unicast udp socket
             */
            void open_unicast();

            /**
             * @brief Open our multicast udp socket
             */
            void open_multicast();

            /**
             * @brief Read a single packet from the given udp file descriptor
             *
             * @param fd the file descriptor to read from
             *
             * @return the data and who it was sent from
             */
            std::pair<sockaddr, std::vector<char>> read_socket(int fd);

            /**
             * @brief Processes the given packet and calls the callback if a packet was completed
             *
             * @param address   who the packet came from
             * @param data      the data that was sent in this packet
             */
            void process_packet(sockaddr&& address, std::vector<char>&& payload);

            /**
             * @brief Remove a target from our list of targets
             *
             * @param t the target to remove
             *
             * @return the next target in the list
             */
            std::list<NetworkTarget>::iterator remove_target(std::list<NetworkTarget>::iterator target);

            /**
             * @brief Sends the passed data packet using the NUClearNetwork
             *
             * @param emit the data to emit
             */
            // void send(const dsl::word::emit::NetworkEmit& emit);

            /// The name of this node in the mesh
            std::string name;
            /// The UDP port that we are listening on
            in_port_t udp_port;

            /// The socket address to send multicast packets to
            sockaddr multicast_target;
            /// The file descriptor for the socket we use to send data and receive unicast data
            int unicast_fd;
            /// The file descriptor for the socket we use to receive multicast data
            int multicast_fd;

            // Our announce packet
            std::vector<char> announce_packet;

            /// An atomic source for packet IDs to make sure they are semi unique
            std::atomic<uint16_t> packet_id_source;

            /// The callback to execute when a data packet is completed
            std::function<void(const NetworkTarget&, const std::array<uint64_t, 2>&, std::vector<char>&&)>
                packet_callback;

            /// The callback to execute when a node joins the network
            std::function<void(std::string, sockaddr)> join_callback;
            /// The callback to execute when a node leaves the network
            std::function<void(std::string, sockaddr)> leave_callback;

            /// A mutex to guard modifications to the target lists
            std::mutex target_mutex;

            /// A list of targets that we are connected to on the network
            std::list<NetworkTarget> targets;

            /// A map of string names to targets with that name
            std::multimap<std::string, std::list<NetworkTarget>::iterator> name_target;

            /// A map of ip/port pairs to the network target they belong to
            std::map<std::pair<in_addr_t, in_port_t>, std::list<NetworkTarget>::iterator> udp_target;
        };

    }  // namespace network
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP
