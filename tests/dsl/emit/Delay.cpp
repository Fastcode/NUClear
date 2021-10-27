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
#include <nuclear>

// Anonymous namespace to keep everything file local
namespace {

struct DelayMessage {};
struct AtTimeMessage {};
struct NormalMessage {};

NUClear::clock::time_point sent;
NUClear::clock::time_point normal_received;
NUClear::clock::time_point delay_received;
NUClear::clock::time_point at_time_received;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {
        emit<Scope::INITIALIZE>(std::make_unique<int>(5));

        // This message should come in later
        on<Trigger<DelayMessage>>().then([this] {
            delay_received = NUClear::clock::now();

            powerplant.shutdown();
        });

        on<Trigger<AtTimeMessage>>().then([] {
            // Don't shut down here we are first
            at_time_received = NUClear::clock::now();
        });

        on<Trigger<NormalMessage>>().then([] { normal_received = NUClear::clock::now(); });

        on<Startup>().then([this] {
            sent = NUClear::clock::now();
            emit(std::make_unique<NormalMessage>());

            // Delay by 200, and a message 100ms in the future, the 200ms one should come in first
            emit<Scope::DELAY>(std::make_unique<DelayMessage>(), std::chrono::milliseconds(200));
            emit<Scope::DELAY>(std::make_unique<AtTimeMessage>(),
                               NUClear::clock::now() + std::chrono::milliseconds(100));
        });
    }
};
}  // namespace

TEST_CASE("Testing the delay emit", "[api][emit][delay]") {
    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    // Ensure the message delays are correct, I would make these bounds tighter, but travis is pretty dumb
    REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(delay_received - sent).count() > 190);
    REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(delay_received - sent).count() < 225);
    REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(at_time_received - sent).count() > 90);
    REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(at_time_received - sent).count() < 120);
}
