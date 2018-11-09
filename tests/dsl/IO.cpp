/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

// Windows can't do this test as it doesn't have file descriptors
#ifndef _WIN32

#include <unistd.h>

#include <nuclear>

namespace {

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)), in(0), out(0) {

        int fds[2];

        if (pipe(static_cast<int*>(fds)) < 0) {
            FAIL("We couldn't make the pipe for the test");
        }

        in  = fds[0];
        out = fds[1];

        on<IO>(in, IO::READ).then([this](const IO::Event& e) {
            // Read from our FD
            unsigned char val;
            ssize_t bytes = ::read(e.fd, &val, 1);

            // Check the data is correct
            REQUIRE((e.events & IO::READ) != 0);
            REQUIRE(bytes == 1);
            REQUIRE(val == 0xDE);

            // Shutdown
            powerplant.shutdown();
        });

        writer = on<IO>(out, IO::WRITE).then([this](const IO::Event& e) {
            // Send data into our fd
            unsigned char val = 0xDE;
            ssize_t bytes     = ::write(e.fd, &val, 1);

            // Check that our data was sent
            REQUIRE((e.events & IO::WRITE) != 0);
            REQUIRE(bytes == 1);

            // Unbind ourselves
            writer.unbind();
        });
    }

    int in;
    int out;
    ReactionHandle writer;
};
}  // namespace

TEST_CASE("Testing the IO extension", "[api][io]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}

#endif
