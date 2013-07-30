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

namespace NUClear {

    template <typename TType>
    void PowerPlant::NetworkMaster::emit(TType* data) {
        
        // Get the hash for this type
        Networking::Hash hash = Networking::hash<TType>();
        
        // Serialize our data
        std::string payload = Networking::Serializer<TType>::serialize(data);
        
        // Create a Message protocol buffer to send
        Networking::NetworkMessage net;
        
        net.set_type(std::string(reinterpret_cast<char*>(hash.data), Networking::Hash::SIZE));
        net.set_source(m_parent->configuration.networkName);
        net.set_payload(payload);
        
        // Serialize our protocol buffer
        std::string serialized = net.SerializeAsString();
        
        // Create a zmq message which holds our hash, and then our data
        zmq::message_t message(serialized.size());
        
        // Copy in our data
        memcpy(message.data(), serialized.data(), serialized.size());
        
        // Send the data after locking the mutex
        std::unique_lock<std::mutex>(m_send);
        m_pub.send(message);
    }
    
    template <typename TType>
    void PowerPlant::NetworkMaster::addType() {
        
        // Get the hash for this type
        Networking::Hash type = Networking::hash<TType>();
        
        // Check if we have already registered this type
        if(m_deserialize.find(type) == std::end(m_deserialize)) {
            
            // Create our deserialization function
            std::function<void (const std::string, const std::string)> parse = [this](const std::string source, const std::string data) {
                
                // Deserialize our data
                TType* parsed = Networking::Serializer<TType>::deserialize(data);
                
                // Wrap our object in a Network object
                Internal::CommandTypes::Network<TType>* event = new Internal::CommandTypes::Network<TType>{source, std::shared_ptr<TType>(parsed)};
                
                // Emit the object
                this->m_parent->reactormaster.emit(event);
            };
            
            // Store our function for use
            m_deserialize.insert(std::make_pair(type, parse));
        }
    }
}
