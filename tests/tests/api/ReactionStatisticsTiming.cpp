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

#include <catch2/catch_test_macros.hpp>
#include <nuclear>

#include "test_util/TestBase.hpp"
#include "test_util/TimeUnit.hpp"
#include "test_util/common.hpp"
#include "util/precise_sleep.hpp"

using TimeUnit = test_util::TimeUnit;

/// Events that occur during the test and the time they occur
/// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::vector<std::pair<std::string, NUClear::clock::time_point>> code_events;
/// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
std::vector<std::pair<std::string, NUClear::clock::time_point>> stat_events;

struct Usage {
    std::map<std::string, std::chrono::steady_clock::duration> real;
    std::map<std::string, NUClear::util::cpu_clock::duration> cpu;
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
Usage usage;

struct DoTest {};
struct HeavyTask {};
struct LightTask {};

const std::string heavy_name   = "Heavy";    // NOLINT(cert-err58-cpp)
const std::string light_name   = "Light";    // NOLINT(cert-err58-cpp)
const std::string initial_name = "Initial";  // NOLINT(cert-err58-cpp)
constexpr int scale            = 5;          // Number of time units to sleep/wait for

using NUClear::message::ReactionEvent;

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment)
        : TestBase(std::move(environment), true, std::chrono::seconds(2)) {

        on<Trigger<Step<1>>, Priority::LOW>().then(initial_name + ":" + heavy_name, [this] {
            code_events.emplace_back("Started " + initial_name + ":" + heavy_name, NUClear::clock::now());
            code_events.emplace_back("Created " + heavy_name, NUClear::clock::now());
            emit(std::make_unique<HeavyTask>());
            code_events.emplace_back("Finished " + initial_name + ":" + heavy_name, NUClear::clock::now());
        });
        on<Trigger<HeavyTask>>().then(heavy_name, [] {
            code_events.emplace_back("Started " + heavy_name, NUClear::clock::now());
            auto start = NUClear::clock::now();
            while (NUClear::clock::now() - start < TimeUnit(scale)) {
            }
            code_events.emplace_back("Finished " + heavy_name, NUClear::clock::now());
        });

        on<Trigger<Step<1>>, Priority::LOW>().then(initial_name + ":" + light_name, [this] {
            code_events.emplace_back("Started " + initial_name + ":" + light_name, NUClear::clock::now());
            code_events.emplace_back("Created " + light_name, NUClear::clock::now());
            emit(std::make_unique<LightTask>());
            code_events.emplace_back("Finished " + initial_name + ":" + light_name, NUClear::clock::now());
        });
        on<Trigger<LightTask>>().then(light_name, [] {
            code_events.emplace_back("Started " + light_name, NUClear::clock::now());
            NUClear::util::precise_sleep(TimeUnit(scale));
            code_events.emplace_back("Finished " + light_name, NUClear::clock::now());
        });

        on<Trigger<ReactionEvent>>().then([](const ReactionEvent& event) {
            const auto& stats = *event.statistics;
            // Check the name ends with light_name or heavy_name
            if (stats.identifiers->name.substr(stats.identifiers->name.size() - light_name.size()) == light_name
                || stats.identifiers->name.substr(stats.identifiers->name.size() - heavy_name.size()) == heavy_name) {

                switch (event.type) {
                    case ReactionEvent::CREATED:
                        stat_events.emplace_back("Created " + stats.identifiers->name, stats.created.nuclear_time);
                        break;
                    case ReactionEvent::STARTED:
                        stat_events.emplace_back("Started " + stats.identifiers->name, stats.started.nuclear_time);
                        break;
                    case ReactionEvent::FINISHED:
                        stat_events.emplace_back("Finished " + stats.identifiers->name, stats.finished.nuclear_time);
                        usage.real[stats.identifiers->name] = stats.finished.real_time - stats.started.real_time;
                        usage.cpu[stats.identifiers->name]  = stats.finished.thread_time - stats.started.thread_time;
                        break;
                    default: break;
                }
            }
        });

        on<Startup>().then("Startup", [this] {
            auto start = NUClear::clock::now();
            code_events.emplace_back("Created " + initial_name + ":" + heavy_name, start);
            code_events.emplace_back("Created " + initial_name + ":" + light_name, start);
            emit(std::make_unique<Step<1>>());
        });
    }
};


TEST_CASE("Testing reaction statistics timing", "[api][reactionstatistics][timing]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<TestReactor>();
    plant.start();

    // Sort the stats events by timestamp as they are not always going to be in order due to how stats are processed
    std::stable_sort(stat_events.begin(), stat_events.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.second < rhs.second;
    });


    auto make_delta = [](const std::vector<std::pair<std::string, NUClear::clock::time_point>>& events) {
        std::vector<std::string> delta_events;
        auto first = events.front().second;
        for (const auto& event : events) {
            auto delta = event.second - first;
            auto units = test_util::round_to_test_units(delta / scale).count();
            delta_events.push_back(event.first + " @ Step " + std::to_string(units));
        }
        return delta_events;
    };

    // Convert the events to delta strings where 1 unit is 1 step unit
    std::vector<std::string> delta_code_events = make_delta(code_events);
    std::vector<std::string> delta_stat_events = make_delta(stat_events);

    const std::vector<std::string> expected = {
        "Created Initial:Heavy @ Step 0",
        "Created Initial:Light @ Step 0",
        "Started Initial:Heavy @ Step 0",
        "Created Heavy @ Step 0",
        "Finished Initial:Heavy @ Step 0",
        "Started Heavy @ Step 0",
        "Finished Heavy @ Step 1",
        "Started Initial:Light @ Step 1",
        "Created Light @ Step 1",
        "Finished Initial:Light @ Step 1",
        "Started Light @ Step 1",
        "Finished Light @ Step 2",
    };


    /* Info Scope */ {
        INFO("Code Events:\n" << test_util::diff_string(expected, delta_code_events));
        REQUIRE(delta_code_events == expected);
    }
    /* Info Scope */ {
        INFO("Statistic Events:\n" << test_util::diff_string(expected, delta_stat_events));
        REQUIRE(delta_stat_events == expected);
    }

    // Most of heavy real time should be cpu time
    REQUIRE(usage.cpu[heavy_name] > usage.real[heavy_name] / 2);

    // Most of light real time should be sleeping
    REQUIRE(usage.cpu[light_name] < usage.real[light_name] / 2);
}
