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

#include "NUClear/PowerPlant.h"

namespace NUClear {
    
    PowerPlant::NetworkMaster::NetworkMaster(PowerPlant* parent) :
    PowerPlant::BaseMaster(parent),
    m_running(true),
    m_context(1),
    m_pub(m_context, ZMQ_PUB),
    m_termPub(m_context, ZMQ_PUB),
    m_sub(m_context, ZMQ_SUB) {
        
        // Get our PGM address
        std::string address = addressForName(m_parent->configuration.networkGroup,
                                             m_parent->configuration.networkPort);
        
        std::cout << "Bound to address " << address << std::endl;
        
        // Bind our publisher to this address
        m_pub.bind(address.c_str());
        
        // Create a secondary inprocess publisher used to terminate the thread
        m_termPub.bind("inproc://networkmaster-term");
        
        // Connect our subscriber to this address and subscribe to all messages
        m_sub.connect(address.c_str());
        m_sub.connect("inproc://networkmaster-term");
        m_sub.setsockopt(ZMQ_SUBSCRIBE, 0, 0);
        
        // Build a task
        Internal::ThreadWorker::ServiceTask task(std::bind(&NetworkMaster::run, this),
                                                  std::bind(&NetworkMaster::kill, this));
        
        m_parent->threadmaster.serviceTask(task);
    }
    
    void PowerPlant::NetworkMaster::run() {
        
        while (m_running) {
            
            zmq::message_t message;
            m_sub.recv(&message);
            
            // If our message size is 0, then it is probably our termination message
            if(message.size() > 0) {
                
                Networking::NetworkMessage proto;
                proto.ParseFromArray(message.data(), message.size());
                
                // Get our hash
                Networking::Hash type;
                memcpy(type.data, proto.type().data(), Networking::Hash::SIZE);
                
                // Find this type's deserializer (if it exists)
                auto it = m_deserialize.find(type);
                if(it != std::end(m_deserialize)) {
                    it->second(proto.source(), proto.payload().data(), proto.payload().size());
                }
            }
        }
    }
    
    void PowerPlant::NetworkMaster::kill() {
        
        // Set our running status to false
        m_running = false;
        
        // Send a message to ensure that our block is released
        zmq::message_t message(0);
        m_termPub.send(message);
    }

    std::string PowerPlant::NetworkMaster::addressForName(std::string name, unsigned port) {
        
        // Hash our string
        Networking::Hash hash = Networking::murmurHash3(name.c_str(), name.size());
        
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
