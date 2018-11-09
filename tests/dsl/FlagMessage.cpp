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
struct SimpleMessage {};

struct MessageA {};
struct MessageB {};

MessageA* a = nullptr;
MessageB* b = nullptr;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {


        on<Trigger<SimpleMessage>>().then([this] {
            auto data = std::make_unique<MessageA>();
            a         = data.get();

            // Emit the first half of the requred data
            emit(data);
        });

        on<Trigger<MessageA>>().then([this] {
            // Check a has been emitted
            REQUIRE(a != nullptr);

            auto data = std::make_unique<MessageB>();
            b         = data.get();

            // Emit the 2nd half
            emit(data);

            // We can shutdown now, the other reactions will process before termination
            powerplant.shutdown();
        });

        on<Trigger<MessageB>>().then([] {
            // Check b has been emitted
            REQUIRE(b != nullptr);
        });

        // We make this high priority to ensure it runs first (will check for more errors)
        on<Trigger<MessageA>, With<MessageB>, Priority::HIGH>().then([](const MessageA&, const MessageB&) {
            // Check A and B have been emitted
            REQUIRE(a != nullptr);
            REQUIRE(b != nullptr);
        });
    }
};
}  // namespace


TEST_CASE("Testing emitting types that are flag types (Have no contents)", "[api][flag]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    auto message = std::make_unique<SimpleMessage>();

    plant.emit(message);

    plant.start();
}
