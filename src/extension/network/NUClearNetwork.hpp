/*
 * MIT License
 *
 * Copyright (c) 2017 NUClear Contributors
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

#ifndef NUCLEAR_EXTENSION_NETWORK_NUCLEAR_NETWORK_HPP
#define NUCLEAR_EXTENSION_NETWORK_NUCLEAR_NETWORK_HPP

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
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

                NetworkTarget(
                    std::string name,
                    const sock_t& target,
                    const std::chrono::steady_clock::time_point& last_update = std::chrono::steady_clock::now())
                    : name(std::move(name)), target(target), last_update(last_update) {

                    // Set our recent packets to an invalid value
                    recent_packets.fill(-1);
                }

                /// The name of the remote target
                std::string name;
                /// The socket address for the remote target
                sock_t target{};
                /// When we last received data from the remote target
                std::chrono::steady_clock::time_point last_update;
                /// A list of the last n packet groups to be received
                std::array<int, std::numeric_limits<uint8_t>::max()> recent_packets{};
                /// An index for the recent_packets (circular buffer)
                std::atomic<uint8_t> recent_packets_index{0};
                /// Mutex to protect the fragmented packet storage
                std::mutex assemblers_mutex;
                /// Storage for fragmented packets while we build them
                std::map<uint16_t,
                         std::pair<std::chrono::steady_clock::time_point, std::map<uint16_t, std::vector<uint8_t>>>>
                    assemblers;

                /// Struct storing the kalman filter for round trip time
                struct RoundTripKF {
                    float process_noise     = 1e-6f;
                    float measurement_noise = 1e-1f;
                    float variance          = 1.0f;
                    float mean              = 1.0f;
                };
                /// A little kalman filter for estimating round trip time
                RoundTripKF round_trip_kf{};

                std::chrono::steady_clock::duration round_trip_time{std::chrono::seconds(1)};

                void measure_round_trip(std::chrono::steady_clock::duration time) {

                    // Make our measurement into a float seconds type
                    const std::chrono::duration<float> m =
                        std::chrono::duration_cast<std::chrono::duration<float>>(time);

                    // Alias variables
                    const auto& Q = round_trip_kf.process_noise;
                    const auto& R = round_trip_kf.measurement_noise;
                    auto& P       = round_trip_kf.variance;
                    auto& X       = round_trip_kf.mean;

                    // Calculate our kalman gain
                    const float K = (P + Q) / (P + Q + R);

                    // Do filter
                    P = R * (P + Q) / (R + P + Q);
                    X = X + (m.count() - X) * K;

                    // Put result into our variable
                    round_trip_time = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                        std::chrono::duration<float>(X));
                }
            };

            NUClearNetwork() = default;
            virtual ~NUClearNetwork();
            NUClearNetwork(const NUClearNetwork& /*other*/)              = delete;
            NUClearNetwork(NUClearNetwork&& /*other*/) noexcept          = delete;
            NUClearNetwork& operator=(const NUClearNetwork& /*rhs*/)     = delete;
            NUClearNetwork& operator=(NUClearNetwork&& /*rhs*/) noexcept = delete;

            /**
             * Send data using the NUClear network.
             *
             * @param hash     The identifying hash for the data
             * @param data     The bytes that are to be sent
             * @param target   Who we are sending to (blank means everyone)
             * @param reliable If the delivery of the data should be ensured
             */
            void send(const uint64_t& hash,
                      const std::vector<uint8_t>& payload,
                      const std::string& target,
                      bool reliable);

            /**
             * Set the callback to use when a data packet is completed.
             *
             * @param f The callback function
             */
            void set_packet_callback(
                std::function<void(const NetworkTarget&, const uint64_t&, const bool&, std::vector<uint8_t>&&)> f);

            /**
             * Set the callback to use when a node joins the network.
             *
             * @param f The callback function
             */
            void set_join_callback(std::function<void(const NetworkTarget&)> f);

            /**
             * Set the callback to use when a node leaves the network.
             *
             * @param f The callback function
             */
            void set_leave_callback(std::function<void(const NetworkTarget&)> f);

            /**
             * Set the callback to use when the system want's to notify when it next needs attention.
             *
             * @param f The callback function
             */
            void set_next_event_callback(std::function<void(std::chrono::steady_clock::time_point)> f);

            /**
             * Leave the NUClear network.
             */
            void shutdown();

            /**
             * Reset our network to use the new settings.
             *
             * Resets the networking system to use the new announce information and name.
             * If the network was already joined, it will first leave and then rejoin the new network.
             * If the provided address is multicast it will join a multicast network.
             * If it is broadcast it will use IPv4 broadcast traffic to announce, unicast addresses will only announce
             * to a single target.
             *
             * @param name         The name of this node in the network
             * @param address      The address to announce on
             * @param port         The port to use for announcement
             * @param bind_address The address to bind to (if unset will bind to all interfaces)
             * @param network_mtu  The mtu of the network we operate on
             */
            void reset(const std::string& name,
                       const std::string& address,
                       in_port_t port,
                       const std::string& bind_address = "",
                       uint16_t network_mtu            = 1500);
            void reset(const std::string& name,
                       const std::string& address,
                       in_port_t port,
                       uint16_t network_mtu = 1500);

            /**
             * Process waiting data in the UDP sockets and send them to the callback if they are relevant.
             */
            void process();

            /**
             * Get the file descriptors that the network listens on.
             *
             * @return A list of file descriptors that the system listens on
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
                std::vector<uint8_t> payload;
            };

            /**
             * Open our data udp socket.
             *
             * @param bind_address The address to bind to or any to bind to all interfaces
             */
            void open_data(const sock_t& bind_address);

            /**
             * Open our announce udp socket.
             *
             * @param announce_target The target to announce to
             * @param bind_address    The address to bind to or any to bind to all interfaces
             */
            void open_announce(const sock_t& announce_target, const sock_t& bind_address);

            /**
             * Processes the given packet and calls the callback if a packet was completed.
             *
             * @param address Who the packet came from
             * @param data    The data that was sent in this packet
             */
            void process_packet(const sock_t& address, std::vector<uint8_t>&& payload);

            /**
             * Send an announce packet to our announce address.
             */
            void announce();

            /**
             * Retransmit waiting packets that failed to send.
             */
            void retransmit();

            /**
             * Send an individual packet to an individual target.
             *
             * @param target    The target to send the packet to
             * @param header    The header for this packet
             * @param packet_no The packet number we are sending
             * @param payload   The data bytes for the entire packet
             * @param reliable  If the packet is reliable (don't drop)
             */
            void send_packet(const sock_t& target,
                             DataPacket header,
                             uint16_t packet_no,
                             const std::vector<uint8_t>& payload,
                             const bool& reliable);

            /**
             * Get the map key for this socket address.
             *
             * @param address Who the packet came from
             *
             * @return The map key for this socket
             */
            static std::array<uint16_t, 9> udp_key(const sock_t& address);

            /**
             * Remove a target from our list of targets.
             *
             * @param t The target to remove
             */
            void remove_target(const std::shared_ptr<NetworkTarget>& target);

            /// The file descriptor for the socket we use to send data and receive regular data
            fd_t data_fd{INVALID_SOCKET};
            /// The file descriptor for the socket we use to receive announce data
            fd_t announce_fd{INVALID_SOCKET};

            /// The largest packet of data we will transmit, based on our IP version and MTU
            uint16_t packet_data_mtu{1000};

            // Our announce packet
            std::vector<uint8_t> announce_packet;

            /// An source for packet IDs to make sure they are semi unique
            uint16_t packet_id_source{0};

            /// The callback to execute when a data packet is completed
            std::function<void(const NetworkTarget&, const uint64_t&, const bool&, std::vector<uint8_t>&&)>
                packet_callback;
            /// The callback to execute when a node joins the network
            std::function<void(const NetworkTarget&)> join_callback;
            /// The callback to execute when a node leaves the network
            std::function<void(const NetworkTarget&)> leave_callback;
            /// The callback to execute when a node leaves the network
            std::function<void(std::chrono::steady_clock::time_point)> next_event_callback;

            /// When we are next due to send an announce packet
            std::chrono::steady_clock::time_point last_announce{std::chrono::seconds(0)};
            /// When the next timed event is due
            std::chrono::steady_clock::time_point next_event{std::chrono::seconds(0)};

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
            std::multimap<std::string, std::shared_ptr<NetworkTarget>, std::less<>> name_target;

            /// A map of ip/port pairs to the network target they belong to
            std::map<std::array<uint16_t, 9>, std::shared_ptr<NetworkTarget>> udp_target;
        };

    }  // namespace network
}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORK_NUCLEAR_NETWORK_HPP
