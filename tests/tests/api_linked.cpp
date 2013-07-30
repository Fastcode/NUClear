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

#include <catch.hpp>

#include "NUClear/NUClear.h"

// Anonymous namespace to keep everything file local
namespace {
    struct SimpleMessage {
        int data;
    };
    
    struct LinkMe {
        int data;
    };
    
    class TestReactor : public NUClear::Reactor {
    public:
        
        TestReactor(NUClear::PowerPlant& plant) : Reactor(plant) {
            on<Trigger<SimpleMessage>>([this](SimpleMessage& message) {
                
                // We check that it's 10 so we don't keep doing this forever
                if(message.data == 10) {
                    
                    // Emit another message of this type with a different value (20)
                    emit(new SimpleMessage{20});
                    
                    // Emit our event that will be linked from
                    emit(new LinkMe{30});
                }
            });
            
            
            // This checks the linked case, the m should be 10 as we are using the linked event
            on<Trigger<LinkMe>, With<Linked<SimpleMessage>>>([this](LinkMe& l, SimpleMessage& m) {
                REQUIRE(m.data == 10);
            });
            
            // This checks the normal case, the m should be 20 as we are using the globally cached event
            on<Trigger<LinkMe>, With<SimpleMessage>>([this](LinkMe& l, SimpleMessage& m) {
                REQUIRE(m.data == 20);
                
                // This is bad but I know that this event will fire second, so I shutdown on this one
                powerPlant.shutdown();
            });
        }
    };
}

TEST_CASE("Testing the Linked<> smart type", "[api][linked]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    
    // I emit the value 10 to get everything started
    plant.emit(new SimpleMessage{10});
    
    plant.start();
}