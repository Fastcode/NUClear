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

#ifndef NUCLEAR_EXTENSIONS_NETWORKING_H
#define NUCLEAR_EXTENSIONS_NETWORKING_H

#include <nuclear>
#include <string>
#include <zmq.hpp>

#include "NetworkMessage.pb.h"
#include "nuclear_bits/extensions/serialization/MurmurHash3.h"

namespace NUClear {
    
    /**
     * @brief This type holds the details for how to deserialze a datatype that we want.
     *
     * @details
     *
     * @author Trent Houliston
     */
    struct NetworkTypeConfig {
        /// @brief The hash for this data type
        extensions::serialization::Hash hash;
        /// @brief The deserializer used to convert the bytes into data
        std::function<void (Reactor& network, const std::string, const std::string)> deserializer;
    };
    
    /**
     * @brief This extension is a handler for network emitting.
     *
     * @details
     *  This handles emitting data to the network using ZMQ. It will take the
     *  passed argument and serize them before emitting them on the network.
     *
     * @author Trent Houliston
     */
    template <typename TData>
    struct PowerPlant::Emit<dsl::Scope::NETWORK, TData> {
        static void emit(PowerPlant& context, std::shared_ptr<TData> data) {
            
            // Make a network message (generic type to trigger on for the networking system)
            auto message = std::make_unique<extensions::serialization::NetworkMessage>();
            
            // Get our hash for this type
            extensions::serialization::Hash hash = extensions::serialization::hash<TData>();
            
            // Serialize our data
            std::string payload = extensions::serialization::Serializer<TData>::serialize(*data);
            
            // Fill our protocol buffer
            message->set_type(std::string(reinterpret_cast<char*>(&hash.data), hash.data.size()));
            message->set_payload(payload);
            
            // Send our data to be emitted
            context.emit(std::move(message));
        };
    };
    
    /**
     * @brief Our extension for when a network type is requested in a trigger
     *
     * @tparam TData the datatype that is being requested from the network
     */
    template <typename TData>
    struct Reactor::Exists<dsl::Network<TData>> {
        static void exists(Reactor& context) {
            
            // Send a direct emit to the Network reactor to tell it about the new type to listen for
            context.emit<Scope::DIRECT>(std::unique_ptr<NetworkTypeConfig>(new NetworkTypeConfig {
                extensions::serialization::hash<TData>(),
                [] (Reactor& reactor, const std::string source, const std::string data) {
                    
                    // Deserialize our data and store it in the network object
                    std::unique_ptr<TData> parsed(std::make_unique<TData>(extensions::serialization::Serializer<TData>::deserialize(data)));
                    std::unique_ptr<Network<TData>> event = std::make_unique<Network<TData>>(source, std::move(parsed));
                    
                    // Emit it to the world
                    reactor.emit(std::move(event));
                }
            }));
        }
    };
    
    namespace extensions {
        
        /**
         * @brief The networking information for this object
         */
        struct NetworkingConfiguration {
            /// @brief the name of this device when packets are sent
            std::string deviceName;
            /// @brief the network device that we are connecting to (a url)
            std::string networkAddress;
        };
        
        class Networking : public Reactor {
        public:
            
            /// @brief This is a globally accessable ZMQ context so that only a single thread must be used for IO
            static zmq::context_t ZMQ_CONTEXT;
            
            /// @brief Build our networking handler reactor
            Networking(std::unique_ptr<Environment> environment);
            
        private:
            /**
             * @brief Starts up the networking reactor listening for packets.
             *
             * @details
             *  Once this is run, packets from the network will be collected and emitted if they
             *  are one of the ones that have been requested by a reaction.
             */
            void run();
            
            /**
             * @brief Kills the networking system, stops the system waiting and kills the sockets.
             */
            void kill();
            
            /// @brief A map of hash types and deseriaizers that can resolve them
            std::unordered_map<serialization::Hash, std::function<void(Reactor&, const std::string, std::string)>> deserialize;
            /// @brief If our system should be running
            volatile bool running;
            /// @brief The name of our device to identify it on outgoing packets
            std::string device;
            /// @brief The address of our network (url)
            std::string address;
            /// @brief A mutex so that we only send a single packet at a time
            std::mutex sendMutex;
            /// @brief The publisher that we used to send data
            zmq::socket_t pub;
            /// @brief The termination publisher, an inproc connection used to stop the system waiting
            zmq::socket_t termPub;
            /// @brief The subscriber, used to get packets from the network
            zmq::socket_t sub;
        };
    }
}

#endif
