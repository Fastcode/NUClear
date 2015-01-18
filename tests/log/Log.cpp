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
    
    class TestReactor : public NUClear::Reactor {
    public:
        
        int testVal = 1337;
        
        TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            
            // Testing that the log message gets through
            on<Trigger<NUClear::message::LogMessage>>().then([this](const NUClear::message::LogMessage& logMessage) {
            
                REQUIRE(logMessage.message == "Got int: 5");
                REQUIRE(logMessage.level == NUClear::DEBUG);
                
                
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                ++testVal = 1338;
            });
            
            // Testing that the log messages are handled before continuing
            
            // Testing that the log level gets through

            on<Trigger<int>>([this](const int& v) {
                
                // Our test val is 1337 to start with
                REQUIRE(testVal == 1337);
                
                // This should pause for 10ms making the second check fail if it does not wait
                log<NUClear::DEBUG>("Got int:", v);
                
                // It should now be 1338
                REQUIRE(testVal == 1338);
                
                // We now try to log a trace (which is below the configured level)
                log<NUClear::TRACE>("Should not log");
                
                // It should still be 1338 (log shouldn't have run)
                REQUIRE(testVal == 1338);
                
                powerplant.shutdown();
            });
        }
    };
}

TEST_CASE("Testing the Log<>() function", "[api][log]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    
    // We are installing with an initial log level of debug
    plant.install<TestReactor, NUClear::DEBUG>();
    
    plant.emit(std::make_unique<int>(5));
    
    plant.start();
}
