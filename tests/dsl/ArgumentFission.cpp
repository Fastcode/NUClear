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

namespace {
    struct BindExtensionTest1 {
        
        static int val1;
        static double val2;
        
        template <typename DSL, typename TFunc>
        static inline std::vector<NUClear::threading::ReactionHandle> bind(NUClear::Reactor&, const std::string&, TFunc&&, int v1, double v2) {
            
            val1 = v1;
            val2 = v2;
            
            return std::vector<NUClear::threading::ReactionHandle>();
        }
    };
    
    int BindExtensionTest1::val1 = 0;
    double BindExtensionTest1::val2 = 0.0;
    
    struct BindExtensionTest2 {
        
        static std::string val1;
        static std::chrono::nanoseconds val2;
        
        template <typename DSL, typename TFunc>
        static inline std::vector<NUClear::threading::ReactionHandle> bind(NUClear::Reactor&, const std::string&, TFunc&&, std::string v1, std::chrono::nanoseconds v2) {
            
            val1 = v1;
            val2 = v2;
            
            return std::vector<NUClear::threading::ReactionHandle>();
        }
    };
    
    std::string BindExtensionTest2::val1 = "";
    std::chrono::nanoseconds BindExtensionTest2::val2 = std::chrono::nanoseconds(0);
    
    struct BindExtensionTest3 {
        
        static int val1;
        static int val2;
        static int val3;
        
        template <typename DSL, typename TFunc>
        static inline std::vector<NUClear::threading::ReactionHandle> bind(NUClear::Reactor&, const std::string&, TFunc&&, int v1, int v2, int v3) {
            
            val1 = v1;
            val2 = v2;
            val3 = v3;
            
            return std::vector<NUClear::threading::ReactionHandle>();
        }
    };
    
    int BindExtensionTest3::val1 = 0;
    int BindExtensionTest3::val2 = 0;
    int BindExtensionTest3::val3 = 0;
    
    struct ShutdownFlag {
    };
    
    class TestReactor : public NUClear::Reactor {
    public:
        TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            
            // Run all three of our extension tests
            on<BindExtensionTest1, BindExtensionTest2, BindExtensionTest3>(5, 7.9, "Hello", std::chrono::seconds(2), 9, 10, 11)
            .then([] {});
            
            REQUIRE(BindExtensionTest1::val1 == 5);
            REQUIRE(BindExtensionTest1::val2 == 7.9);
            
            REQUIRE(BindExtensionTest2::val1 == "Hello");
            REQUIRE(BindExtensionTest2::val2.count() == std::chrono::nanoseconds(2 * std::nano::den).count());
            
            REQUIRE(BindExtensionTest3::val1 == 9);
            REQUIRE(BindExtensionTest3::val2 == 10);
            REQUIRE(BindExtensionTest3::val3 == 11);
            
            // Try another ordering
            
            // Have our trigger that kills everything
            
            on<Trigger<ShutdownFlag>>()
            .then([this] {
                
                // We are finished the test
                powerplant.shutdown();
            });
        }
    };
}

TEST_CASE("Testing distributing arguments to multiple bind functions (NUClear Fission)", "[api][dsl][fission]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    
    plant.emit(std::make_unique<ShutdownFlag>());
    
    plant.start();
}