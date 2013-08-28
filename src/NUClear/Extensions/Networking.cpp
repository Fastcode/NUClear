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

#include "NUClear/Extensions/Networking.h"

#include <sstream>

namespace NUClear {
    namespace Extensions {
        zmq::context_t Networking::ZMQ_CONTEXT(1);

        Networking::Networking(PowerPlant* parent) : Reactor(parent),
        running(true),
        pub(ZMQ_CONTEXT, ZMQ_PUB),
        termPub(ZMQ_CONTEXT, ZMQ_PUB),
        sub(ZMQ_CONTEXT, ZMQ_SUB) {

            // Get our PGM address
            std::string address = addressForName(parent->configuration.networkGroup,
                                                 parent->configuration.networkPort);

            std::cout << "Network bound to address " << address << std::endl;

            // Bind our publisher to this address
            pub.bind(address.c_str());

            // Create a secondary inprocess publisher used to terminate the thread
            termPub.bind("inproc://networkmaster-term");

            // Connect our subscriber to this address and subscribe to all messages
            sub.connect(address.c_str());
            sub.connect("inproc://networkmaster-term");
            sub.setsockopt(ZMQ_SUBSCRIBE, 0, 0);

            // Build a task
            Internal::ThreadWorker::ServiceTask task(std::bind(&Networking::run, this),
                                                     std::bind(&Networking::kill, this));

            powerPlant->addServiceTask(task);

            // We emit our zmq context here so that other classes are able to use it as well (if they need zmq)
            
            

            on<Trigger<NetworkTypeConfig>>([this] (const NetworkTypeConfig& config) {

                // Check if we have already registered this type
                if(deserialize.find(config.hash) == std::end(deserialize)) {
                    // Store our function for use
                    deserialize.insert(std::make_pair(config.hash, config.deserializer));
                }
            });

            on<Trigger<Serialization::NetworkMessage>>([this] (const Serialization::NetworkMessage& message) {

                // Serialize our protocol buffer
                std::string serialized = message.SerializeAsString();

                // Create a zmq message which holds our hash, and then our data
                zmq::message_t zmsg(serialized.size());

                // Copy in our data
                memcpy(zmsg.data(), serialized.data(), serialized.size());
                
                // Send the data after locking the mutex
                std::lock_guard<std::mutex> lock(sendMutex);
                pub.send(zmsg);
            });
        }

        void Networking::run() {

            while (running) {

                zmq::message_t message;
                sub.recv(&message);

                // If our message size is 0, then it is probably our termination message
                if(message.size() > 0) {

                    Serialization::NetworkMessage proto;
                    proto.ParseFromArray(message.data(), message.size());

                    // Get our hash
                    Serialization::Hash type;
                    memcpy(type.data, proto.type().data(), Serialization::Hash::SIZE);

                    // Find this type's deserializer (if it exists)
                    auto it = deserialize.find(type);
                    if(it != std::end(deserialize)) {
                        it->second(this, proto.source(), proto.payload());
                    }
                }
            }
        }

        void Networking::kill() {

            // Set our running status to false
            running = false;

            // Send a message to ensure that our block is released
            zmq::message_t message(0);
            termPub.send(message);
        }

        std::string Networking::addressForName(std::string name, unsigned port) {

            // Hash our string
            Serialization::Hash hash = Serialization::murmurHash3(name.c_str(), name.size());

            // Convert our hash into a multicast address
            uint32_t addr = 0xE0000200 + (hash.hash() % (0xEFFFFFFF - 0xE0000200));
            
            // Split our mulitcast address into a epgm address
            std::stringstream out;
            
            // Build our string
            out << "epgm://"
            << ((addr >> 24) & 0xFF) << "."
            << ((addr >> 16) & 0xFF) << "."
            << ((addr >>  8) & 0xFF) << "."
            << (addr & 0xFF) << ":"
            << port;
            
            return out.str();
        }
    }
}
