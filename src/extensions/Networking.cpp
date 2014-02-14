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

#include "nuclear_bits/extensions/Networking.h"

#include <sstream>

namespace NUClear {
    namespace extensions {
        zmq::context_t Networking::ZMQ_CONTEXT(1);
        
        Networking::Networking(std::unique_ptr<Environment> environment) : Reactor(std::move(environment)),
        running(true),
        device("default"),
        address(""),
        pub(ZMQ_CONTEXT, ZMQ_PUB),
        termPub(ZMQ_CONTEXT, ZMQ_PUB),
        sub(ZMQ_CONTEXT, ZMQ_SUB) {
            
            // Create a secondary inprocess publisher used to terminate the thread
            termPub.bind("inproc://networkmaster-term");
            
            // Connect our subscriber to this address and subscribe to all messages
            sub.connect("inproc://networkmaster-term");
            
            // This is a ZMQ subscriber, and it does not filter
            sub.setsockopt(ZMQ_SUBSCRIBE, 0, 0);
            
            // Build a task
            threading::ThreadWorker::ServiceTask task(std::bind(&Networking::run, this),
                                                      std::bind(&Networking::kill, this));
            
            powerPlant->addServiceTask(task);
            
            // When we get a network type configuration, we must build a lambda to handle it
            on<Trigger<NetworkTypeConfig>>([this] (const NetworkTypeConfig& config) {
                
                // Check if we have already registered this type
                if(deserialize.find(config.hash) == std::end(deserialize)) {
                    // Store our function for use
                    deserialize.insert(std::make_pair(config.hash, config.deserializer));
                }
            });
            
            // When we get a network configuration object, we connect to it
            on<Trigger<NetworkingConfiguration>>([this] (const NetworkingConfiguration& config) {
                
                // Disconnect from our old address if it was connected
                if(address != config.networkAddress && !address.empty()) {
                    
                    log<DEBUG>("Network disconnecting from ", address);
                    
                    pub.disconnect(address.c_str());
                    sub.disconnect(address.c_str());
                }
                
                // Change our device name
                device = config.deviceName;
                
                if(address != config.networkAddress && !config.networkAddress.empty()) {
                    
                    // Change our network address
                    address = config.networkAddress;
                    
                    log<DEBUG>("Network connecting to ", address, " as ", device);
                    
                    // Connect to the new address
                    pub.connect(address.c_str());
                    sub.connect(address.c_str());
                }
            });
            
            // If we get a network message
            on<Trigger<serialization::NetworkMessage>>([this] (serialization::NetworkMessage message) {
                
                // Set our packets source
                message.set_source(device);
                
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
            
            // Until we exit
            while (running) {
                
                zmq::message_t message;
                sub.recv(&message);
                
                // If our message size is 0, then it is probably our termination message
                if(message.size() > 0) {
                    
                    // Parse our message
                    serialization::NetworkMessage proto;
                    proto.ParseFromArray(message.data(), message.size());
                    
                    // Get our hash
                    serialization::Hash type;
                    memcpy(&type.data, proto.type().data(), type.data.size());
                    
                    // Find this type's deserializer (if it exists)
                    auto it = deserialize.find(type);
                    if(it != std::end(deserialize)) {
                        it->second(this, proto.source(), proto.payload());
                    }
                }
            }
            
            // Close everything
            pub.close();
            sub.close();
            termPub.close();
        }
        
        void Networking::kill() {
            
            // Set our running status to false
            running = false;
            
            // Send a message to ensure that our block is released
            zmq::message_t message(0);
            termPub.send(message);
        }
    }
}
