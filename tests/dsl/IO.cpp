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

extern "C" {
    #include <unistd.h>
}

#include "nuclear"

namespace {
    
    class TestReactor : public NUClear::Reactor {
    public:
        TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
            
            int fds[2];
            
            if(pipe(fds) < 0) {
                FAIL("We couldn't make the pipe for the test");
            }
            
            in = fds[0];
            out = fds[1];
            
            on<IO>(in, IO::READ).then([this] {
                
                // Check the data recieved is the same as sent
                std::cout << "Read Task" << std::endl;
                
                // Shutdown
                powerplant.shutdown();
            });
            
            writer = on<IO>(out, IO::WRITE).then([this] {
                // Send data into read
                
                char val = 0xDE;
                
                write(out, &val, 1);
                
                std::cout << "Write Task" << std::endl;
                
                // Unbind ourselves
                writer.unbind();
            })[0];
        }
        
        int in;
        int out;
        ReactionHandle writer;
    };
}

TEST_CASE("Testing the IO extension", "[api]") {
    
    NUClear::PowerPlant::Configuration config;
    config.threadCount = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    
    plant.start();
}
