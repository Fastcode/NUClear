/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    struct StartTest {};
    struct Synced {};
    template <int id>
    struct PoolFinished {};

    static constexpr int POOL_COUNT = 10;

    template <int id>
    struct TestPool {
        static constexpr int concurrency = 1;
    };

    void register_pool_callbacks(NUClear::util::Sequence<> /*unused*/) {}

    template <int ID, int... IDs>
    void register_pool_callbacks(NUClear::util::Sequence<ID, IDs...> /*unused*/) {
        on<Trigger<Synced>, Pool<TestPool<ID>>, Sync<TestReactor>>().then([this] {
            add_event("Pool Message");
            emit(std::make_unique<PoolFinished<ID>>());
        });

        register_pool_callbacks(NUClear::util::Sequence<IDs...>());
    }

    template <int... IDs>
    void register_callbacks(NUClear::util::Sequence<IDs...> /*unused*/) {
        register_pool_callbacks(NUClear::util::Sequence<IDs...>());

        on<Trigger<PoolFinished<IDs>>...>().then([this] {
            add_event("Finished");
            powerplant.shutdown();
        });
    }

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Startup>().then([this] {
            add_event("Startup");
            emit(std::make_unique<StartTest>());
        });

        // Trigger on the start message and emit a message that will be synced
        on<Trigger<StartTest>, Sync<TestReactor>>().then([this] {
            add_event("Send Synced Message");
            emit(std::make_unique<Synced>());
        });

        // Register all the callbacks and a final callback to gather the results
        register_callbacks(NUClear::util::GenerateSequence<0, POOL_COUNT>());
    }

    /// Add an event to the event list
    void add_event(const std::string& event) {
        static std::mutex events_mutex;
        const std::lock_guard<std::mutex> lock(events_mutex);
        events.push_back(event);
    }

    /// A vector of events that have happened
    std::vector<std::string> events;
};


TEST_CASE("Test that if a pool has nothing to do because of a sync group it will recover", "[api][pool][group]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    std::vector<std::string> expected = {
        "Startup",
        "Send Synced Message",
    };
    for (int i = 0; i < TestReactor::POOL_COUNT; ++i) {
        expected.push_back("Pool Message");
    }
    expected.push_back("Finished");

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, reactor.events));

    // Check the events fired in order and only those events
    REQUIRE(reactor.events == expected);
}
