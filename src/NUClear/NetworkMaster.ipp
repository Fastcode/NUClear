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
        zmq::message_t message;
        
        m_pub.send(data, sizeof(TType));
    }
    
    template <typename TType>
    void PowerPlant::NetworkMaster::addType() {
        
        std::string type = typeid(TType).name();
        
        // TODO here is where we make our void lambda that can interpret this type it deserializes and emits
        // TODO we also need to know the source of the message so we can send that too
        std::function<void (void*)> parse = [this](void* data) {
            TType* parsed = Networking::Serializer<TType>::deserialize(data);
            this->m_parent->reactormaster.emit(new int(1));
        };
        
        // TODO put this parse function into an unordered map
        
        // This is how we subscribe to a paticular message type, the "B" is the message type
        m_sub.setsockopt(ZMQ_SUBSCRIBE, type.c_str(), 1);
    }
}
