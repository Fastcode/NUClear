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

namespace {

struct Message {
    int val;
    Message(int val) : val(val){};
};

struct ShutdownOnIdle {};

std::atomic<int> counter(0);

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Trigger<Message>, Sync<TestReactor>>().then([this](const Message& m) {
            // Match message value against counter value
            auto value = counter.load();

            // Increment our counter
            ++counter;

            CHECK(value == m.val);
        });

        on<Trigger<ShutdownOnIdle>, Priority::IDLE>().then([this] { powerplant.shutdown(); });

        on<Startup>().then([this] {
            emit(std::make_unique<Message>(0));
            emit(std::make_unique<Message>(1));
            emit(std::make_unique<Message>(2));
            emit(std::make_unique<Message>(3));
            emit(std::make_unique<ShutdownOnIdle>());
        });
    }
};
}  // namespace

TEST_CASE("Testing that the Sync priority queue word works correctly", "[api][sync][priority]") {
    NUClear::PowerPlant::Configuration config;
    config.thread_count = 2;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    // Repeat the test to try and hit the race condition
    auto i = GENERATE(Catch::Generators::range(0, 100));
    INFO("Testing iteration " << (i + 1));
    {
        plant.start();

        // Reset the counter
        auto value = counter.load();
        counter.store(0);

        REQUIRE(value == 4);
    }
}
