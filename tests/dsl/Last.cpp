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

struct TestMessage {
    int value;

    TestMessage(int v) : value(v){};
};

int emit_counter = 0;
int recv_counter = 0;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Last<5, Trigger<TestMessage>>>().then([this](std::list<std::shared_ptr<const TestMessage>> messages) {
            // We got another one
            ++recv_counter;

            // Send out another before we test
            emit(std::make_unique<TestMessage>(++emit_counter));

            // Finish when we get to 10
            if (messages.front()->value >= 10) { powerplant.shutdown(); }
            else {
                // Our list must be less than 5 long
                REQUIRE(messages.size() <= 5);

                // If our size is less than 5 it should be the size of the front element
                if (messages.size() < 5) { REQUIRE(messages.size() == messages.back()->value); }

                // Check that our numbers are decreasing
                int i = messages.front()->value;
                for (auto& m : messages) {
                    REQUIRE(m->value == i);
                    ++i;
                }
            }
        });

        on<Startup>().then([this] { emit(std::make_unique<TestMessage>(++emit_counter)); });
    }
};
}  // namespace

TEST_CASE("Testing the last n feature", "[api][last]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();
}
