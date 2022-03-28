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

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

// Setup use of the bounds-checked function if it is available
#ifdef __STDC_LIB_EXT1__
#    define __STDC_WANT_LIB_EXT1__ 1
#endif

// This define declares that we are using system_clock as the base clock for NUClear
#define NUCLEAR_CLOCK_TYPE std::chrono::system_clock

#include <chrono>
#include <nuclear>
#include <utility>

#include "message/ReactionStatistics.hpp"

// Anonymous namespace to keep everything file local
namespace {

std::vector<std::pair<NUClear::clock::time_point, std::chrono::system_clock::time_point>> times;
constexpr int n_time = 100;

class TestReactor : public NUClear::Reactor {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Running every this slowed down clock should execute slower
        on<Every<10, std::chrono::milliseconds>>().then([this] {
            if (times.size() > n_time) { powerplant.shutdown(); }
        });

        on<Trigger<NUClear::message::ReactionStatistics>>().then(
            [this](const NUClear::message::ReactionStatistics& stats) {
                times.push_back(std::make_pair(stats.emitted, std::chrono::system_clock::now()));
            });
    }
};
}  // namespace

TEST_CASE("Testing base clock works correctly", "[api][base_clock]") {

    STATIC_REQUIRE(std::is_same<NUClear::clock, std::chrono::system_clock>::value);

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);

    // We are installing with an initial log level of debug
    plant.install<TestReactor>();

    plant.start();

    struct TimeData {
        TimeData(const std::tm* tm)
            : year(tm->tm_year)
            , month(tm->tm_mon)
            , day(tm->tm_mday)
            , hour(tm->tm_hour)
            , min(tm->tm_min)
            , sec(tm->tm_sec) {}
        bool operator==(const TimeData& other) {
            return year == other.year && month == other.month && day == other.day && hour == other.hour
                   && min == other.min && sec == other.sec;
        }
        int year;
        int month;
        int day;
        int hour;
        int min;
        int sec;
    };
    // Compute the differences between the time pairs
    int match_count = 0;
    for (const auto& time_pairs : times) {
        std::time_t ntt = NUClear::clock::to_time_t(time_pairs.first);
        std::time_t stt = NUClear::clock::to_time_t(time_pairs.second);

#ifdef __STDC_LIB_EXT1__
        std::tm result;

        std::localtime_s(&ntt, &result);
        TimeData nuclear_clock(&result);

        std::localtime_s(&stt, &result);
        TimeData local_clock(&result);
#else
        TimeData nuclear_clock(std::localtime(&ntt));
        TimeData local_clock(std::localtime(&stt));
#endif  // __STDC_LIB_EXT1__

        UNSCOPED_INFO("Year.: " << nuclear_clock.year + 1900 << " == " << local_clock.year + 1900 << "\n"
                                << "Month: " << nuclear_clock.month << " == " << local_clock.month << "\n"
                                << "Day..: " << nuclear_clock.day << " == " << local_clock.day << "\n"
                                << "Hour.: " << nuclear_clock.hour << " == " << local_clock.hour << "\n"
                                << "Min..: " << nuclear_clock.min << " == " << local_clock.min << "\n"
                                << "Sec..: " << nuclear_clock.sec << " == " << local_clock.sec);
        if (nuclear_clock == local_clock) { ++match_count; }
    }

    // At least 95% of all reaction statistics should match to the second
    REQUIRE(match_count >= 95);
}
