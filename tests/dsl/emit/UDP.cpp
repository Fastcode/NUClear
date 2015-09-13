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

#include <catch.hpp>

#include "nuclear"

// Anonymous namespace to keep everything file local
namespace {
    
    int receivedMessages = 0;
    
    class TestReactor : public NUClear::Reactor {
    public:
        
        TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            emit<Scope::INITIALIZE>(std::make_unique<int>(5));
            
            int boundPort;
            std::tie(std::ignore, boundPort) = on<UDP>().then([this, &boundPort] (const UDP::Packet& packet) {
                ++receivedMessages;
                
                switch(packet.data[0]) {
                    case 'a':
                    case 'b':
                        REQUIRE(packet.source.address == INADDR_LOOPBACK);
                        REQUIRE(packet.dest.address == INADDR_LOOPBACK);
                        REQUIRE(packet.dest.port == boundPort);
                        break;
                    case 'c':
                        REQUIRE(packet.source.address == INADDR_LOOPBACK);
                        REQUIRE(packet.source.port == 12345);
                        REQUIRE(packet.dest.address == INADDR_LOOPBACK);
                        REQUIRE(packet.dest.port == boundPort);
                        break;
                    case 'd':
                        REQUIRE(packet.source.address == INADDR_LOOPBACK);
                        REQUIRE(packet.source.port == 54321);
                        REQUIRE(packet.dest.address == INADDR_LOOPBACK);
                        REQUIRE(packet.dest.port == boundPort);
                        break;
                }
                
                if(receivedMessages == 4) {
                    powerplant.shutdown();
                }
            });
            
            on<Startup>().then([boundPort, this] {
                // Send using a string
                emit<Scope::UDP>(std::make_unique<char>('a'), "127.0.0.1", boundPort);
                emit<Scope::UDP>(std::make_unique<char>('b'), INADDR_LOOPBACK, boundPort);
                emit<Scope::UDP>(std::make_unique<char>('c'), "127.0.0.1", boundPort, INADDR_ANY, 12345);
                emit<Scope::UDP>(std::make_unique<char>('d'), INADDR_LOOPBACK, boundPort, INADDR_ANY, 54321);
            });
        }
    };
}

TEST_CASE("Testing UDP emits work correctly", "[api][emit][udp]") {
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}
