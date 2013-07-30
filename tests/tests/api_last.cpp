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

#include "NUClear.h"

// Anonymous namespace to keep everything file local
namespace {
    
    struct TestData {
        int data;
    };
    
    class TestReactor : public NUClear::Reactor {
    public:
        
        TestReactor(NUClear::PowerPlant& plant) : Reactor(plant) {
            // Trigger every 10 milliseconds
            on<Trigger<Last<5, TestData>>>([this](const std::vector<std::shared_ptr<const TestData>>& data) {
                
                // Check that we have the same number of elements as our first data or 5 elements
                REQUIRE(data.size() == std::min(5, data.front()->data));
                
                for (unsigned i = 1; i < data.size(); ++i) {
                    // Check that the value is correct
                    REQUIRE(data[i - 1]->data == data[i]->data + 1);
                }
                
                // We will continue until we have done 10 elements
                if (data.front()->data < 10) {
                    emit(new TestData{data.front()->data + 1});
                }
                else {
                    // We are finished the test
                    this->powerPlant.shutdown();
                }
            });
        }
    };
}

TEST_CASE("Testing the Last<> Smart Type", "[api][last]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    
    plant.emit(new TestData{1});
    
    plant.start();
}