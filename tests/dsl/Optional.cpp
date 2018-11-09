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

int trigger1 = 0;
int trigger2 = 0;
int trigger3 = 0;
int trigger4 = 0;

struct MessageA {};

template <typename R, typename... A>
auto resolve_function_type(R (*)(A...)) -> R (*)(A...);
struct MessageB {};

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Trigger<MessageA>, With<MessageB>>().then([](const MessageA&, const MessageB&) {
            ++trigger1;
            FAIL("This should never run as MessageB is never emitted");
        });

        on<Trigger<MessageA>, Optional<With<MessageB>>>().then(
            [this](const MessageA&, const std::shared_ptr<const MessageB>& b) {
                ++trigger2;

                switch (trigger2) {
                    case 1:
                        // On our trigger, b should not exist
                        REQUIRE(!b);
                        break;
                    default: FAIL("Trigger 2 was triggered more than once");
                }

                // Emit B to start the second set
                emit(std::make_unique<MessageB>());
            });

        on<Trigger<MessageB>, With<MessageA>>().then([] {
            // This should run once
            ++trigger3;
        });

        // Double trigger test (to ensure that it can handle multiple DSL words
        on<Optional<Trigger<MessageA>, Trigger<MessageB>>>().then(
            [this](const std::shared_ptr<const MessageA>& a, const std::shared_ptr<const MessageB>& b) {
                ++trigger4;
                switch (trigger4) {
                    case 1:
                        // Check that A exists and B does not
                        REQUIRE(a);
                        REQUIRE(!b);
                        break;
                    case 2:
                        // Check that both exist
                        REQUIRE(a);
                        REQUIRE(b);

                        // We should be done now
                        powerplant.shutdown();
                        break;

                    default: FAIL("Trigger 4 should only be triggered twice"); break;
                }
            });

        on<Startup>().then([this] {
            // Emit only message A
            emit(std::make_unique<MessageA>());
        });
    }
};
}  // namespace

TEST_CASE("Testing that optional is able to let data through even if it's invalid", "[api][optional]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    plant.start();

    // Check that it was all as expected
    REQUIRE(trigger1 == 0);
    REQUIRE(trigger2 == 1);
    REQUIRE(trigger3 == 1);
    REQUIRE(trigger4 == 2);
}
