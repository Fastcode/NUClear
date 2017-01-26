/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

struct DelayMessage {};
struct NormalMessage {};

NUClear::clock::time_point sent;
NUClear::clock::time_point normal_received;
NUClear::clock::time_point delay_received;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
        emit<Scope::INITIALIZE>(std::make_unique<int>(5));

        // This message should come in 500ms later
        on<Trigger<DelayMessage>>().then([this] {
            delay_received = NUClear::clock::now();

            powerplant.shutdown();
        });

        on<Trigger<NormalMessage>>().then([this] { normal_received = NUClear::clock::now(); });

        on<Startup>().then([this] {
            sent = NUClear::clock::now();
            emit(std::make_unique<NormalMessage>());
            emit<Scope::DELAY>(std::make_unique<DelayMessage>(), std::chrono::milliseconds(200));
        });
    }
};
}

TEST_CASE("Testing the delay emit", "[api][emit][delay]") {
    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    // Ensure the delayed message is about 200ms
    REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(delay_received - sent).count() > 180);
}
