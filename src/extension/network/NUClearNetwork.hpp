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

#ifndef NUCLEAR_EXTENSION_NETWORK_NUCLEARNETWORK_HPP
#define NUCLEAR_EXTENSION_NETWORK_NUCLEARNETWORK_HPP

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "../../util/network/sock_t.hpp"
#include "../../util/platform.hpp"
#include "wire_protocol.hpp"

namespace NUClear {
namespace extension {
    namespace network {

        class NUClearNetwork {
        private:
            using sock_t = util::network::sock_t;

        public:
            struct NetworkTarget {

                NetworkTarget(std::string name,
                              sock_t target,
                              std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now())
                    : name(name)
                    , target(target)
                    , last_update(last_update)
                    , recent_packets()
                    , recent_packets_index(0)
                    , assemblers_mutex()
                    , assemblers()
                    , round_trip_kf()
                    , round_trip_time(std::chrono::seconds(1)) {

                    // Set our recent packets to an invalid value
                    recent_packets.fill(-1);
                }

                /// The name of the remote target
                std::string name;
                /// The socket address for the remote target
                sock_t target;
                /// When we last received data from the remote target
                std::chrono::steady_clock::time_point last_update;
                /// A list of the last n packet groups to be received
                std::array<int, std::numeric_limits<uint8_t>::max()> recent_packets;
                /// An index for the recent_packets (circular buffer)
                std::atomic<uint8_t> recent_packets_index;
                /// Mutex to protect the fragmented packet storage
                std::mutex assemblers_mutex;
                /// Storage for fragmented packets while we build them
                std::map<uint16_t,
                         std::pair<std::chrono::steady_clock::time_point, std::map<uint16_t, std::vector<char>>>>
                    assemblers;

                /// A little kalman filter for estimating round trip time
                struct RoundTripKF {
                    float process_noise     = 1e-6f;
                    float measurement_noise = 1e-1f;
                    float variance          = 1.0f;
                    float mean              = 1.0f;
                } round_trip_kf;

                std::chrono::steady_clock::duration round_trip_time;

                inline void measure_round_trip(std::chrono::steady_clock::duration time) {

                    // Make our measurement into a float seconds type
                    std::chrono::duration<float, std::ratio<1>> m =
                        std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1>>>(time);

                    // Alias variables
                    auto& Q = round_trip_kf.process_noise;
                    auto& R = round_trip_kf.measurement_noise;
                    auto& P = round_trip_kf.variance;
                    auto& X = round_trip_kf.mean;

                    // Calculate our kalman gain
                    float K = (P + Q) / (P + Q + R);

                    // Do filter
                    P = R * (P + Q) / (R + P + Q);
                    X = X + (m.count() - X) * K;

                    // Put result into our variable
                    round_trip_time = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                        std::chrono::duration<float, std::ratio<1>>(X));
                }
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
            void send(const uint64_t& hash, const std::vector<char>& payload, const std::string& target, bool reliable);

            /**
             * @brief Set the callback to use when a data packet is completed
             *
             * @param f the callback function
             */
            void set_packet_callback(
                std::function<void(const NetworkTarget&, const uint64_t&, const bool&, std::vector<char>&&)> f);

            /**
             * @brief Set the callback to use when a node joins the network
             *
             * @param f the callback function
             */
            void set_join_callback(std::function<void(const NetworkTarget&)> f);

            /**
             * @brief Set the callback to use when a node leaves the network
             *
             * @param f the callback function
             */
            void set_leave_callback(std::function<void(const NetworkTarget&)> f);

            /**
             * @brief Set the callback to use when the system want's to notify when it next needs attention
             *
             * @param f the callback function
             */
            void set_next_event_callback(std::function<void(std::chrono::steady_clock::time_point)> f);

            /**
             * @brief Leave the NUClear network
             */
            void shutdown();

            /**
             * @brief Reset our network to use the new settings
             *
             * @details
             *  Resets the networking system to use the new announce information and name.
             *  If the network was already joined, it will first leave and then rejoin the new network.
             *  If the provided address is multicast it will join a multicast network. If it is broadcast
             *  it will use IPv4 broadcast traffic to announce, unicast addresses will only announce to a single target.
             *
             * @param name          the name of this node in the network
             * @param address       the address to announce on
             * @param port          the port to use for announcement
             * @param network_mtu   the mtu of the network we operate on
             */
            void reset(const std::string& name,
                       const std::string& address,
                       in_port_t port,
                       uint16_t network_mtu = 1500);

            /**
             * @brief Process waiting data in the UDP sockets and send them to the callback if they are relevant.
             */
            void process();

            /**
             * @brief Get the file descriptors that the network listens on
             *
             * @return a list of file descriptors that the system listens on
             */
            std::vector<fd_t> listen_fds();

        private:
            struct PacketQueue {

                struct PacketTarget {

                    /// Constructor a new PacketTarget
                    PacketTarget(std::weak_ptr<NetworkTarget> target, std::vector<uint8_t> acked);

                    /// The target we are sending this packet to
                    std::weak_ptr<NetworkTarget> target;

                    /// The bitset of the packets that have been acked
                    std::vector<uint8_t> acked;

                    /// When we last sent data to this client
                    std::chrono::steady_clock::time_point last_send;
                };

                /// Default constructor for the PacketQueue
                PacketQueue();

                /// The remote targets that want this packet
                std::list<PacketTarget> targets;

                /// The header of the packet to send
                DataPacket header;

                /// The data to send
                std::vector<char> payload;
            };

            /**
             * @brief Open our data udp socket
             */
            void open_data(const sock_t& announce_target);

            /**
             * @brief Open our announce udp socket
             */
            void open_announce(const sock_t& announce_target);

            /**
             * @brief Read a single packet from the given udp file descriptor
             *
             * @param fd the file descriptor to read from
             *
             * @return the data and who it was sent from
             */
            std::pair<sock_t, std::vector<char>> read_socket(fd_t fd);

            /**
             * @brief Processes the given packet and calls the callback if a packet was completed
             *
             * @param address   who the packet came from
             * @param data      the data that was sent in this packet
             */
            void process_packet(const sock_t& address, std::vector<char>&& payload);

            /**
             * @brief Send an announce packet to our announce address
             */
            void announce();

            /**
             * @brief Retransmit waiting packets that failed to send
             */
            void retransmit();

            /**
             * @brief Send an individual packet to an individual target
             *
             * @param target    the target to send the packet to
             * @param header    the header for this packet
             * @param packet_no the packet number we are sending
             * @param payload   the data bytes for the entire packet
             * @param reliable  if the packet is reliable (don't drop)
             */
            void send_packet(const sock_t& target,
                             DataPacket header,
                             uint16_t packet_no,
                             const std::vector<char>& payload,
                             const bool& reliable);

            /**
             * @brief Get the map key for this socket address
             *
             * @param address   who the packet came from
             *
             * @return the map key for this socket
             */
            static std::array<uint16_t, 9> udp_key(const sock_t& address);

            /**
             * @brief Remove a target from our list of targets
             *
             * @param t the target to remove
             */
            void remove_target(const std::shared_ptr<NetworkTarget>& target);

            /// The file descriptor for the socket we use to send data and receive regular data
            fd_t data_fd;
            /// The file descriptor for the socket we use to receive announce data
            fd_t announce_fd;

            /// The largest packet of data we will transmit, based on our IP version and MTU
            uint16_t packet_data_mtu;

            // Our announce packet
            std::vector<char> announce_packet;

            /// An atomic source for packet IDs to make sure they are semi unique
            std::atomic<uint16_t> packet_id_source;

            /// The callback to execute when a data packet is completed
            std::function<void(const NetworkTarget&, const uint64_t&, const bool&, std::vector<char>&&)>
                packet_callback;
            /// The callback to execute when a node joins the network
            std::function<void(const NetworkTarget&)> join_callback;
            /// The callback to execute when a node leaves the network
            std::function<void(const NetworkTarget&)> leave_callback;
            /// The callback to execute when a node leaves the network
            std::function<void(std::chrono::steady_clock::time_point)> next_event_callback;

            /// When we are next due to send an announce packet
            std::chrono::steady_clock::time_point last_announce;
            /// When the next timed event is due
            std::chrono::steady_clock::time_point next_event;

            /// A mutex to guard modifications to the target lists
            /// NOTE: mutex lock order must always be this order to avoid deadlocks
            std::mutex target_mutex;
            /// A mutex to guard modifications to the send queue
            std::mutex send_queue_mutex;

            /// A map from packet_id to allow resending reliable data
            std::map<uint16_t, PacketQueue> send_queue;

            /// A list of targets that we are connected to on the network
            std::list<std::shared_ptr<NetworkTarget>> targets;

            /// A map of string names to targets with that name
            std::multimap<std::string, std::shared_ptr<NetworkTarget>> name_target;

            /// A map of ip/port pairs to the network target they belong to
            std::map<std::array<uint16_t, 9>, std::shared_ptr<NetworkTarget>> udp_target;
        };

    }  // namespace network
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP
