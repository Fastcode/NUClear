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

#include <catch.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"

namespace {

/// @brief A vector of events that have happened
std::vector<std::string> events;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

struct StartTest {};
struct Synced {};
template <int id>
struct PoolFinished {};

static constexpr int POOL_COUNT = 10;

class TestReactor : public test_util::TestBase<TestReactor, 1000> {
public:
    template <int id>
    struct TestPool {
        static constexpr int thread_count = 1;
    };

    template <int... ID>
    void register_callbacks(NUClear::util::Sequence<ID...>) {

        NUClear::util::unpack(on<Trigger<Synced>, Pool<TestPool<ID>>, Sync<TestReactor>>().then([this] {
            events.push_back("Pool Message");
            emit(std::make_unique<PoolFinished<ID>>());
        })...);

        on<Trigger<PoolFinished<ID>>...>().then([this] {
            events.push_back("Finished");
            powerplant.shutdown();
        });
    }

    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment), false) {

        on<Startup>().then([this] {
            events.push_back("Startup");
            emit(std::make_unique<StartTest>());
        });

        // Trigger on the start message and emit a message that will be synced
        on<Trigger<StartTest>, Sync<TestReactor>>().then([this] {
            events.push_back("Send Synced Message");
            emit(std::make_unique<Synced>());
        });

        // Register all the callbacks and a final callback to gather the results
        register_callbacks(NUClear::util::GenerateSequence<0, POOL_COUNT>());
    }
};

}  // namespace


TEST_CASE("Test that if a pool has nothing to do because of a sync group it will recover", "[api][pool][group]") {

    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    std::vector<std::string> expected = {"Send Synced Message"};
    for (int i = 0; i < POOL_COUNT; ++i) {
        expected.push_back("Pool Message");
    }
    expected.push_back("Finished");

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, events));

    // Check the events fired in order and only those events
    REQUIRE(events == expected);
}
