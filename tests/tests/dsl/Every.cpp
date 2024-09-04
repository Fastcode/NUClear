/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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
#include <numeric>

#include "test_util/TestBase.hpp"
#include "test_util/common.hpp"

class TestReactor : public test_util::TestBase<TestReactor> {
public:
    TestReactor(std::unique_ptr<NUClear::Environment> environment)
        : TestBase(std::move(environment), false, std::chrono::seconds(10)) {

        // Trigger on 3 different types of every
        on<Every<1000, Per<std::chrono::seconds>>>().then([this]() { every_times.push_back(NUClear::clock::now()); });
        on<Every<1, std::chrono::milliseconds>>().then([this]() { per_times.push_back(NUClear::clock::now()); });
        on<Every<>>(std::chrono::milliseconds(1)).then([this]() { dynamic_times.push_back(NUClear::clock::now()); });

        // Gather data for some amount of time
        on<Watchdog<TestReactor, 5, std::chrono::seconds>>().then([this] { powerplant.shutdown(); });
    }

    /// Times that the every trigger has been called
    std::vector<NUClear::clock::time_point> every_times;
    /// Times that the every per trigger has been called
    std::vector<NUClear::clock::time_point> per_times;
    /// Times that the dynamic every trigger has been called
    std::vector<NUClear::clock::time_point> dynamic_times;
};

void test_results(const std::vector<NUClear::clock::time_point>& times) {

    // Build up our difference vector
    std::vector<double> diff;
    for (size_t i = 0; i < times.size() - 1; ++i) {
        const double delta = std::chrono::duration_cast<std::chrono::duration<double>>(times[i + 1] - times[i]).count();

        // Calculate the difference between the expected and actual time
        diff.push_back(delta - 1e-3);
    }

    // Calculate our mean, range, and stddev for the set
    const double sum      = std::accumulate(std::begin(diff), std::end(diff), 0.0);
    const double mean     = sum / double(diff.size());
    const double variance = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    const double stddev   = std::sqrt(variance / double(diff.size() - 1));

    // As time goes on the average wait should be close to 0
    INFO("Average error in timing: " << mean << "±" << stddev);

    REQUIRE(std::abs(mean) < 0.0005);
    REQUIRE(std::abs(mean + stddev * 2) < 0.008);
}


TEST_CASE("Testing the Every<> DSL word", "[api][every][per]") {

    NUClear::Configuration config;
    config.default_pool_concurrency = 1;
    NUClear::PowerPlant plant(config);
    test_util::add_tracing(plant);
    plant.install<NUClear::extension::ChronoController>();
    const auto& reactor = plant.install<TestReactor>();
    plant.start();

    {
        INFO("Testing Every");
        test_results(reactor.every_times);
    }

    {
        INFO("Testing Every Per");
        test_results(reactor.per_times);
    }

    {
        INFO("Testing Dynamic Every every");
        test_results(reactor.dynamic_times);
    }
}
