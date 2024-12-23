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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>
#include <random>

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    template <int I>
    struct Message {};

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        // Declare in the order you'd expect them to fire
        on<Trigger<Message<1>>, Priority::REALTIME>().then([this] { events.push_back("Realtime Message<1>"); });
        on<Trigger<Message<1>>, Priority::HIGH>().then("High", [this] { events.push_back("High Message<1>"); });
        on<Trigger<Message<1>>>().then([this] { events.push_back("Default Message<1>"); });
        on<Trigger<Message<1>>, Priority::NORMAL>().then("Normal", [this] { events.push_back("Normal Message<1>"); });
        on<Trigger<Message<1>>, Priority::LOW>().then("Low", [this] { events.push_back("Low Message<1>"); });
        on<Trigger<Message<1>>, Priority::IDLE>().then([this] { events.push_back("Idle Message<1>"); });

        // Declare in the opposite order to what you'd expect them to fire
        on<Trigger<Message<2>>, Priority::IDLE>().then([this] { events.push_back("Idle Message<2>"); });
        on<Trigger<Message<2>>, Priority::LOW>().then([this] { events.push_back("Low Message<2>"); });
        on<Trigger<Message<2>>, Priority::NORMAL>().then([this] { events.push_back("Normal Message<2>"); });
        on<Trigger<Message<2>>>().then([this] { events.push_back("Default Message<2>"); });
        on<Trigger<Message<2>>, Priority::HIGH>().then([this] { events.push_back("High Message<2>"); });
        on<Trigger<Message<2>>, Priority::REALTIME>().then([this] { events.push_back("Realtime Message<2>"); });

        // Declare in a random order
        std::array<int, 5> order = {0, 1, 2, 3, 4};
        std::shuffle(order.begin(), order.end(), std::mt19937(std::random_device()()));
        for (const auto& i : order) {
            switch (i) {
                case 0:
                    on<Trigger<Message<3>>, Priority::REALTIME>().then(
                        [this] { events.push_back("Realtime Message<3>"); });
                    break;
                case 1:
                    on<Trigger<Message<3>>, Priority::HIGH>().then([this] { events.push_back("High Message<3>"); });
                    break;
                case 2:
                    on<Trigger<Message<3>>, Priority::NORMAL>().then([this] { events.push_back("Normal Message<3>"); });
                    on<Trigger<Message<3>>>().then([this] { events.push_back("Default Message<3>"); });
                    break;
                case 3:
                    on<Trigger<Message<3>>, Priority::LOW>().then([this] { events.push_back("Low Message<3>"); });
                    break;
                case 4:
                    on<Trigger<Message<3>>, Priority::IDLE>().then([this] { events.push_back("Idle Message<3>"); });
                    break;
                default: throw std::invalid_argument("Should be impossible");
            }
        }

        on<Startup>().then([this] {
            emit(std::make_unique<Message<1>>());
            emit(std::make_unique<Message<2>>());
            emit(std::make_unique<Message<3>>());
        });
    }

    /// Events that occur during the test
    std::vector<std::string> events;
};


TEST_CASE("Tests that priority orders the tasks appropriately", "[api][priority]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    const std::vector<std::string> expected = {
        "Realtime Message<1>",
        "Realtime Message<2>",
        "Realtime Message<3>",
        "High Message<1>",
        "High Message<2>",
        "High Message<3>",
        "Default Message<1>",
        "Normal Message<1>",
        "Normal Message<2>",
        "Default Message<2>",
        "Normal Message<3>",
        "Default Message<3>",
        "Low Message<1>",
        "Low Message<2>",
        "Low Message<3>",
        "Idle Message<1>",
        "Idle Message<2>",
        "Idle Message<3>",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
