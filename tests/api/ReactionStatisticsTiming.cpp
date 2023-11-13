/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

// Anonymous namespace to keep everything file local
namespace {

/// @brief Events that occur during the test and the time they occur
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::vector<std::pair<std::string, NUClear::clock::time_point>> events;

struct Usage {
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::map<std::string, NUClear::clock::duration> real;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::map<std::string, NUClear::util::user_cpu_clock::duration> user;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    std::map<std::string, NUClear::util::kernel_cpu_clock::duration> kernel;
};
Usage usage;

struct DoTest {};
struct HeavyTask {};
struct LightTask {};

constexpr std::chrono::milliseconds STEP = std::chrono::milliseconds(100);
const std::string heavy_name             = "Heavy";
const std::string light_name             = "Light";
const std::string initial_name           = "Initial";

using NUClear::message::ReactionStatistics;

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : TestBase(std::move(environment)) {

        // This reaction is here to emit something from a ReactionStatistics trigger
        // This shouldn't cause reaction statistics of their own otherwise everything would explode
        on<Trigger<Step<1>>>().then(initial_name, [this] {
            events.emplace_back("Code: Started " + initial_name, NUClear::clock::now());

            events.emplace_back("Code: Emit " + heavy_name, NUClear::clock::now());
            emit(std::make_unique<HeavyTask>());
            events.emplace_back("Code: Emitted " + heavy_name, NUClear::clock::now());

            // Wait a step to separate out the start times (and the heavy task execution time)
            std::this_thread::sleep_for(STEP);

            events.emplace_back("Code: Emit " + light_name, NUClear::clock::now());
            emit(std::make_unique<LightTask>());
            events.emplace_back("Code: Emit " + light_name, NUClear::clock::now());

            events.emplace_back("Code: Finished " + initial_name, NUClear::clock::now());
        });

        on<Trigger<HeavyTask>>().then(heavy_name, [] {
            events.emplace_back("Code: Started " + heavy_name, NUClear::clock::now());

            // Wait using CPU power
            auto start = NUClear::clock::now();
            while (NUClear::clock::now() - start < STEP) {
            }

            events.emplace_back("Code: Finished " + heavy_name, NUClear::clock::now());
        });

        on<Trigger<LightTask>>().then(light_name, [] {
            events.emplace_back("Code: Started " + light_name, NUClear::clock::now());

            // Wait by sleeping
            std::this_thread::sleep_for(STEP);

            events.emplace_back("Code: Finished " + light_name, NUClear::clock::now());
        });


        on<Trigger<ReactionStatistics>>().then([this](const ReactionStatistics& stats) {
            if (stats.identifiers.reactor == reactor_name
                && (stats.identifiers.name == initial_name || stats.identifiers.name == heavy_name
                    || stats.identifiers.name == light_name)) {
                events.emplace_back("Stat: Emitted " + stats.identifiers.name, stats.emitted);
                events.emplace_back("Stat: Started " + stats.identifiers.name, stats.started);
                events.emplace_back("Stat: Finished " + stats.identifiers.name, stats.finished);

                usage.real[stats.identifiers.name]   = stats.finished - stats.started;
                usage.user[stats.identifiers.name]   = stats.user_cpu_time;
                usage.kernel[stats.identifiers.name] = stats.kernel_cpu_time;
            }
        });

        on<Startup>().then("Startup", [this] { emit(std::make_unique<Step<1>>()); });
    }
};
}  // namespace

TEST_CASE("Testing reaction statistics timing", "[api][reactionstatistics][timing]") {

    NUClear::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactor>();
    plant.start();

    // The events are added in a different order due to stats running after, so sort it to be in the same order
    std::sort(events.begin(), events.end(), [](const auto& lhs, const auto& rhs) { return lhs.second < rhs.second; });

    // Convert the events to delta strings where 1 unit is 1 step unit
    std::vector<std::string> delta_events;
    auto first = events.front().second;
    for (auto& event : events) {
        auto delta = event.second - first;
        auto units = std::chrono::duration_cast<std::chrono::nanoseconds>(delta).count()
                     / std::chrono::duration_cast<std::chrono::nanoseconds>(STEP).count();
        delta_events.push_back(event.first + " @ Step " + std::to_string(units));
    }

    const std::vector<std::string> expected = {
        "Stat: Emitted Initial @ Step 0",  "Stat: Started Initial @ Step 0",  "Code: Started Initial @ Step 0",
        "Code: Emit Heavy @ Step 0",       "Stat: Emitted Heavy @ Step 0",    "Code: Emitted Heavy @ Step 0",
        "Code: Emit Light @ Step 1",       "Stat: Emitted Light @ Step 1",    "Code: Emit Light @ Step 1",
        "Code: Finished Initial @ Step 1", "Stat: Finished Initial @ Step 1", "Stat: Started Heavy @ Step 1",
        "Code: Started Heavy @ Step 1",    "Code: Finished Heavy @ Step 2",   "Stat: Finished Heavy @ Step 2",
        "Stat: Started Light @ Step 2",    "Code: Started Light @ Step 2",    "Code: Finished Light @ Step 3",
        "Stat: Finished Light @ Step 3",
    };

    // Make an info print the diff in an easy to read way if we fail
    INFO(test_util::diff_string(expected, delta_events));

    // Check the events fired in order and only those events
    REQUIRE(delta_events == expected);

    // Check that the amount of CPU time spent is at least reasonable for each of the reactions

    // Most of initial real time should be spent sleeping
    REQUIRE(usage.user[initial_name] + usage.kernel[initial_name] < usage.real[initial_name] / 2);

    // Most of heavy real time should be cpu time
    REQUIRE(usage.user[heavy_name] + usage.kernel[heavy_name] > usage.real[heavy_name] / 2);

    // Most of light real time should be sleeping
    REQUIRE(usage.user[light_name] + usage.kernel[light_name] < usage.real[light_name] / 2);
}
