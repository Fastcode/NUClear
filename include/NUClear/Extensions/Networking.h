/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
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

#include <string>
#include <zmq.hpp>

#include "NUClear.h"
#include "NUClear/NetworkMessage.pb.h"
#include "NUClear/Serialization/MurmurHash3.h"

namespace NUClear {

    struct NetworkTypeConfig {
        Serialization::Hash hash;
        std::function<void (Reactor* network, const std::string, const std::string)> deserializer;
    };
    
    // Our emit overload for a networked emit
    template <typename TData>
    struct PowerPlant::Emit<Internal::CommandTypes::Scope::NETWORK, TData> {
        static void emit(PowerPlant* context, std::shared_ptr<TData> data) {

            auto message = std::make_unique<Serialization::NetworkMessage>();

            // Get our hash for this type
            Serialization::Hash hash = Serialization::hash<TData>();

            // Serialize our data
            std::string payload = Serialization::Serializer<TData>::serialize(*data);

            // Fill our protocol buffer
            message->set_type(std::string(reinterpret_cast<char*>(hash.data), Serialization::Hash::SIZE));
            message->set_source(context->configuration.networkName);
            message->set_payload(payload);

            // Send our data to be emitted
            context->emit(std::move(message));
        };
    };

    // Our Exists overload to start listening for paticular network datatypes
    template <typename TData>
    struct Reactor::Exists<Internal::CommandTypes::Network<TData>> {
        static void exists(Reactor* context) {

            // Send a direct emit to the Network reactor to tell it about the new type to listen for
            context->emit<Scope::DIRECT>(std::unique_ptr<NetworkTypeConfig>(new NetworkTypeConfig {
                Serialization::hash<TData>(),
                [] (Reactor* reactor, const std::string source, const std::string data) {

                    // Deserialize our data and store it in the network object
                    std::unique_ptr<TData> parsed(std::make_unique<TData>(Serialization::Serializer<TData>::deserialize(data)));
                    std::unique_ptr<Network<TData>> event = std::make_unique<Network<TData>>(source, std::move(parsed));

                    // Emit it to the world
                    reactor->emit(std::move(event));
                }
            }));
        }
    };

    namespace Extensions {
        class Networking : public Reactor {
        public:
            static zmq::context_t ZMQ_CONTEXT;

            /// @brief TODO
            Networking(std::unique_ptr<Environment> environment);

        private:
            /**
             * @brief TODO
             *
             * @details
             *  TODO
             */
            void run();

            /**
             * @brief TODO
             *
             * @details
             *  TODO
             */
            void kill();

            /**
             * @brief TODO
             *
             * @details
             *  TODO
             *
             * @param name
             * @param port
             */
            static std::string addressForName(const std::string name, const unsigned port);

            /// @brief TODO
            std::unordered_map<Serialization::Hash, std::function<void(Reactor*, const std::string, std::string)>> deserialize;
            /// @brief TODO
            volatile bool running;
            /// @brief TODO
            std::mutex sendMutex;
            /// @brief TODO
            zmq::socket_t pub;
            /// @brief TODO
            zmq::socket_t termPub;
            /// @brief TODO
            zmq::socket_t sub;
        };
    }
}

#endif
