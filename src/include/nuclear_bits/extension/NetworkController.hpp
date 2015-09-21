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

#ifndef NUCLEAR_EXTENSION_NETWORKCONTROLLER
#define NUCLEAR_EXTENSION_NETWORKCONTROLLER

#include "nuclear"

namespace NUClear {
    namespace extension {
        
        class NetworkController : public Reactor {
        private:
            struct NetworkTarget {
                
                NetworkTarget(std::string name
                              , uint32_t address
                              , uint16_t tcpPort
                              , uint16_t udpPort
                              , int tcpFD)
                : name(name)
                , address(address)
                , tcpPort(tcpPort)
                , udpPort(udpPort)
                , tcpFD(tcpFD) {}
                
                std::string name;
                uint32_t address;
                int tcpPort;
                int udpPort;
                int tcpFD;
                ReactionHandle handle;
                std::map<uint16_t, std::pair<clock::time_point, std::vector<std::vector<char>>>> buffer;
                std::mutex bufferMutex;
            };
            
        public:
            explicit NetworkController(std::unique_ptr<NUClear::Environment> environment);
            
        private:
            void tcpConnection(const TCP::Connection& con);
            void tcpHandler(const IO::Event& event);
            void udpHandler(const UDP::Packet& packet);
            void tcpSend(const dsl::word::emit::NetworkEmit& emit);
            void udpSend(const dsl::word::emit::NetworkEmit& emit);
            void announce();
            
            static constexpr const int MAX_UDP_PAYLOAD_LENGTH = 1024;
            static constexpr const int MAX_NUM_UDP_ASSEMBLEY = 5;
            
            std::mutex writeMutex;
            
            ReactionHandle udpHandle;
            ReactionHandle tcpHandle;
            ReactionHandle multicastHandle;
            ReactionHandle multicastEmitHandle;
            ReactionHandle networkEmitHandle;
            
            std::string name;
            std::string multicastGroup;
            int multicastPort;
            int udpPort;
            int tcpPort;
            
            int udpServerFD;
            int tcpServerFD;
            
            std::atomic<uint16_t> packetIDSource;
            
            std::mutex reactionMutex;
            std::multimap<std::array<uint64_t, 2>, std::shared_ptr<threading::Reaction>> reactions;
            
            std::mutex targetMutex;
            std::list<NetworkTarget> targets;
            std::multimap<std::string, std::list<NetworkTarget>::iterator> nameTarget;
            std::map<std::pair<int, int>, std::list<NetworkTarget>::iterator> udpTarget;
            std::map<int, std::list<NetworkTarget>::iterator> tcpTarget;
        };
    }
}

#endif
