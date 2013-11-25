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
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "NUClear.h"


// Anonymous namespace to keep everything file local
namespace {
    
    struct SimpleMessage {
        int data;
    };
    
    class TestReactor : public NUClear::Reactor {
    public:
        TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            
            on<Trigger<SimpleMessage>>([this](const SimpleMessage& message) {
                
                // The message we recieved should have test == 10
                REQUIRE(message.data == 10);
                
                // We are finished the test
                this->powerPlant->shutdown();
            });
        }
    };
}

TEST_CASE("A very basic test for Emit and On", "[api]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    
    plant.emit(std::unique_ptr<SimpleMessage>(new SimpleMessage{10}));
    
    plant.start();
}

namespace {
    
    struct DifferentOrderingMessage1 {};
    struct DifferentOrderingMessage2 {};
    struct DifferentOrderingMessage3 {};
    
    class DifferentOrderingReactor : public NUClear::Reactor {
    public:
        DifferentOrderingReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            // Check that the lists are combined, and that the function args are in order
            on<With<DifferentOrderingMessage1>, Trigger<DifferentOrderingMessage3>, With<DifferentOrderingMessage2>>
            ([this](const DifferentOrderingMessage1& m1, const DifferentOrderingMessage3& m2, const DifferentOrderingMessage2& m3) {
                this->powerPlant->shutdown();
            });
        }
    };
}

TEST_CASE("Testing poorly ordered on arguments", "[api]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<DifferentOrderingReactor>();
    
    plant.emit(std::make_unique<DifferentOrderingMessage1>());
    plant.emit(std::make_unique<DifferentOrderingMessage2>());
    plant.emit(std::make_unique<DifferentOrderingMessage3>());
    
    plant.start();
}
