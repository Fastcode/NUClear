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
    
    struct TestObject {
        int x;
    };
    
    class TestReactor : public NUClear::Reactor {
    public:
        // Store our times
        std::vector<std::chrono::time_point<std::chrono::steady_clock>> times;
        
        TestReactor(NUClear::PowerPlant& plant) : Reactor(plant) {
            
            // Trigger on a networked event
            on<Trigger<Network<TestObject>>>([this](const Network<TestObject>& message) {
                REQUIRE(message.data->x == 5);
            });
        }
    };
}

TEST_CASE("api/network/basic", "Testing the Networking system") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    
    plant.install<TestReactor>();
    
    for (int i = 0; i < 50; ++i) {
        plant.networkEmit(new TestObject{5});
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    plant.start();
}