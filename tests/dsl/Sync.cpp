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

template <int i>
struct Message {
    int val;
    Message(int val) : val(val){};
};


std::atomic<int> semaphore(0);
int finished = 0;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Trigger<Message<0>>, Sync<TestReactor>>().then([this](const Message<0>& m) {
            // Increment our semaphore
            ++semaphore;

            // Sleep for some time to be safe
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // Check we got the right message
            REQUIRE(m.val == 123);

            // Require our semaphore is 1
            REQUIRE(semaphore == 1);

            // Emit a message 1 here, it should not run yet
            emit(std::make_unique<Message<1>>(10));

            // Sleep for some time again
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // Decrement our semaphore
            --semaphore;
        });

        on<Trigger<Message<0>>, Sync<TestReactor>>().then([this](const Message<0>& m) {
            // Increment our semaphore
            ++semaphore;

            // Sleep for some time to be safe
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // Check we got the right message
            REQUIRE(m.val == 123);

            // Require our semaphore is 1
            REQUIRE(semaphore == 1);

            // Emit a message 1 here, it should not run yet
            emit(std::make_unique<Message<1>>(10));

            // Sleep for some time again
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // Decrement our semaphore
            --semaphore;
        });

        on<Trigger<Message<1>>, Sync<TestReactor>>().then([this](const Message<1>& m) {
            // Increment our semaphore
            ++semaphore;

            // Sleep for some time to be safe
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // Check we got the right message
            REQUIRE(m.val == 10);

            // Require our semaphore is 1
            REQUIRE(semaphore == 1);

            // Sleep for some time again
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // Decrement our semaphore
            --semaphore;

            if (++finished == 2) {
                powerplant.shutdown();
            }
        });

        on<Startup>().then([this] { emit(std::make_unique<Message<0>>(123)); });
    }
};
}  // namespace

TEST_CASE("Testing that the Sync word works correctly", "[api][sync]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 4;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}
