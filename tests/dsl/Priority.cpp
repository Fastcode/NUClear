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

struct Message1 {};
struct Message2 {};
struct Message3 {};

bool low  = false;
bool med  = false;
bool high = false;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        on<Trigger<Message3>, Priority::HIGH>().then("High", [] {
            // We should be the first to run
            REQUIRE(!low);
            REQUIRE(!med);
            REQUIRE(!high);

            high = true;
        });

        on<Trigger<Message2>, Priority::NORMAL>().then("Normal", [] {
            // We should be the second to run
            REQUIRE(!low);
            REQUIRE(!med);
            REQUIRE(high);

            med = true;
        });

        on<Trigger<Message1>, Priority::LOW>().then("Low", [this] {
            // We should be the final one to run
            REQUIRE(!low);
            REQUIRE(med);
            REQUIRE(high);

            low = true;

            // We're done
            powerplant.shutdown();
        });
    }
};
}  // namespace


TEST_CASE("Tests that priority orders the tasks appropriately", "[api][priority]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();

    // Emit message 2, then 1 then 3 (totally wrong order)
    // Should require the priority queue to sort it out
    plant.emit(std::make_unique<Message2>());
    plant.emit(std::make_unique<Message1>());
    plant.emit(std::make_unique<Message3>());

    plant.start();

    // Make sure everything ran
    REQUIRE(low);
    REQUIRE(med);
    REQUIRE(high);
}
