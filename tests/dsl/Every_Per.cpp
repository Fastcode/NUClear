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
#include <numeric>

namespace {

class TestReactorPer : public NUClear::Reactor {
public:
    // Store our times
    std::vector<NUClear::clock::time_point> times;

    static constexpr size_t NUM_LOG_ITEMS = 1000;

    static constexpr size_t CYCLES_PER_SECOND = 1000;

    TestReactorPer(std::unique_ptr<NUClear::Environment> environment) : Reactor(std::move(environment)) {

        // Trigger every 10 milliseconds
        on<Every<CYCLES_PER_SECOND, Per<std::chrono::seconds>>>().then([this]() {
            // Start logging our times each time an emit happens
            times.push_back(NUClear::clock::now());

            // Once we have enough items then we can do our statistics
            if (times.size() == NUM_LOG_ITEMS) {

                // Build up our difference vector
                std::vector<double> diff;

                for (size_t i = 0; i < times.size() - 1; ++i) {
                    std::chrono::nanoseconds delta = times[i + 1] - times[i];

                    // Store our difference in seconds
                    diff.push_back(double(delta.count()) / double(std::nano::den));
                }

                // Normalize our differences to jitter
                for (double& d : diff) {
                    d -= 1.0 / double(CYCLES_PER_SECOND);
                }

                // Calculate our mean, range, and stddev for the set
                double sum      = std::accumulate(std::begin(diff), std::end(diff), 0.0);
                double mean     = sum / double(diff.size());
                double variance = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
                double stddev   = std::sqrt(variance / double(diff.size()));

                // As time goes on the average wait should be 0 (we accept less then 0.5ms for this test)
                REQUIRE(fabs(mean) < 0.0005);

                // Require that 95% (ish) of all results are fast enough
                REQUIRE(fabs(mean + stddev * 2) < 0.008);
            }
            // Once we have more then enough items then we shutdown the powerplant
            else if (times.size() > NUM_LOG_ITEMS) {
                // We are finished the test
                this->powerplant.shutdown();
            }
        });
    }
};
}  // namespace

TEST_CASE("Testing the Every<> Smart Type using Per", "[api][every][per]") {

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);
    plant.install<TestReactorPer>();

    plant.start();
}
