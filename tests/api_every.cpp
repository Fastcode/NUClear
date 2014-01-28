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

#include "nuclear"

// Anonymous namespace to keep everything file local
namespace {
    
    class TestReactor : public NUClear::Reactor {
    public:
        // Store our times
        std::vector<NUClear::clock::time_point> times;
        
        TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            // Trigger every 10 milliseconds
            on<Trigger<Every<10, std::chrono::milliseconds>>>([this](const time_t& message) {
                
                // Store 11 times (10 durations)
                if(times.size() < 11) {
                    times.push_back(message);
                }
                
                // Check that the 10 times are about 10ms each (within 1ms)
                // and that all of them are within 1ms of 100ms
                else if (times.size() == 11) {
                    std::chrono::nanoseconds total = std::chrono::nanoseconds(0);
                    
                    for(int i = 0; i < abs(times.size() - 1); ++i) {
                        std::chrono::nanoseconds wait = times[i + 1] - times[i];
                        total += wait;
                        
                        std::chrono::milliseconds test = std::chrono::duration_cast<std::chrono::milliseconds>(wait);
                        
                        // Check that our local drift is within 1ms
                        REQUIRE(abs(10 - test.count()) <= 1);
                    }
                    
                    std::chrono::milliseconds test = std::chrono::duration_cast<std::chrono::milliseconds>(total);
                    
                    // Check that our total drift is also within 1ms
                    REQUIRE(abs(100 - test.count()) <= 1);
                    
                    // We are finished the test
                    this->powerPlant->shutdown();
                }
            });
        }
    };

    class TestReactorPer : public NUClear::Reactor {
    public:
        // Store our times
        std::vector<NUClear::clock::time_point> times;
        
        TestReactorPer(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            // Trigger every 10 milliseconds
            on<Trigger<Every<10, Per<std::chrono::seconds>>>>([this](const time_t& message) {
                
                // Store 11 times (10 durations)
                if(times.size() < 11) {
                    times.push_back(message);
                }
                
                // Check that the 10 times are about 10ms each (within 1ms)
                // and that all of them are within 1ms of 100ms
                else if (times.size() == 11) {
                    std::chrono::nanoseconds total = std::chrono::nanoseconds(0);
                    
                    for(int i = 0; i < abs(times.size() - 1); ++i) {
                        std::chrono::nanoseconds wait = times[i + 1] - times[i];
                        total += wait;
                        
                        std::chrono::milliseconds test = std::chrono::duration_cast<std::chrono::milliseconds>(wait);
                        
                        // Check that our local drift is within 1ms
                        REQUIRE(abs(10 - test.count()) <= 1);
                    }
                    
                    std::chrono::milliseconds test = std::chrono::duration_cast<std::chrono::milliseconds>(total);
                    
                    // Check that our total drift is also within 1ms
                    REQUIRE(abs(100 - test.count()) <= 1);
                    
                    // We are finished the test
                    this->powerPlant->shutdown();
                }
            });
        }
    };

}

TEST_CASE("Testing the Every<> Smart Type", "[api][every]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    
    plant.start();
}

TEST_CASE("Testing the Every<> Smart Type using Per", "[api][every]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactorPer>();
    
    plant.start();
}
