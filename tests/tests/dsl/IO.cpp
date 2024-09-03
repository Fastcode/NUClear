/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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

#include <array>
#include <catch2/catch_test_macros.hpp>

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

// Windows can't do this test as it doesn't have file descriptors
#ifndef _WIN32

    #include <unistd.h>

    #include <nuclear>

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        std::array<int, 2> fds{-1, -1};

        if (::pipe(fds.data()) < 0) {
            return;
        }
        in  = fds[0];
        out = fds[1];

        // Set the pipe to non-blocking
        ::fcntl(in.get(), F_SETFL, ::fcntl(in.get(), F_GETFL) | O_NONBLOCK);
        ::fcntl(out.get(), F_SETFL, ::fcntl(out.get(), F_GETFL) | O_NONBLOCK);

        on<IO>(in.get(), IO::READ | IO::CLOSE).then([this](const IO::Event& e) {
            if ((e.events & IO::READ) != 0) {
                // Read from our fd
                char c{0};
                for (auto bytes = ::read(e.fd, &c, 1); bytes > 0; bytes = ::read(e.fd, &c, 1)) {
                    // If we read something, log it
                    if (bytes > 0) {
                        read_events.push_back("Read " + std::to_string(bytes) + " bytes (" + c + ") from pipe");
                    }
                }
            }

            // FD was closed
            if ((e.events & IO::CLOSE) != 0) {
                read_events.push_back("Closed pipe");
                powerplant.shutdown();
            }
        });

        writer = on<IO>(out.get(), IO::WRITE).then([this](const IO::Event& e) {
            // Send data into our fd
            const char c       = "Hello"[char_no];
            const ssize_t sent = ::write(e.fd, &c, 1);

            // If we wrote something, log it and move to the next character
            if (sent > 0) {
                ++char_no;
                write_events.push_back("Wrote " + std::to_string(sent) + " bytes (" + c + ") to pipe");
            }

            if (char_no == 5) {
                ::close(e.fd);
            }
        });
    }

    NUClear::util::FileDescriptor in;
    NUClear::util::FileDescriptor out;
    int char_no{0};
    ReactionHandle writer;

    /// Events that occur during the test reading
    std::vector<std::string> read_events;
    /// Events that occur during the test writing
    std::vector<std::string> write_events;
};


TEST_CASE("Testing the IO extension", "[api][io]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<NUClear::extension::IOController>();
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> read_expected = {
        "Read 1 bytes (H) from pipe",
        "Read 1 bytes (e) from pipe",
        "Read 1 bytes (l) from pipe",
        "Read 1 bytes (l) from pipe",
        "Read 1 bytes (o) from pipe",
        "Closed pipe",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO("Read Events\n" << test_util::diff_string(read_expected, reactor.read_events));

    const std::vector<std::string> write_expected{
        "Wrote 1 bytes (H) to pipe",
        "Wrote 1 bytes (e) to pipe",
        "Wrote 1 bytes (l) to pipe",
        "Wrote 1 bytes (l) to pipe",
        "Wrote 1 bytes (o) to pipe",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO("Write Events\n" << test_util::diff_string(write_expected, reactor.write_events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.read_events == read_expected);
    REQUIRE(reactor.write_events == write_expected);
}

#else


TEST_CASE("Testing the IO extension", "[api][io]") {
    SUCCEED("This test is not supported on Windows");
}

#endif  // _WIN32
