/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
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

#include "nuclearnet/RTTEstimator.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>

using NUClear::network::RTTEstimator;
using namespace std::chrono_literals;

SCENARIO("RTTEstimator initial timeout is 1 second", "[nuclearnet][rtt]") {
    RTTEstimator rtt;
    // RFC 6298: initial RTO should be generous (we use 1s default)
    auto timeout = rtt.timeout();
    REQUIRE(timeout >= 900ms);
    REQUIRE(timeout <= 1100ms);
}

SCENARIO("RTTEstimator converges towards measured RTT", "[nuclearnet][rtt]") {
    RTTEstimator rtt;

    // Simulate stable 50ms RTT for several measurements
    for (int i = 0; i < 20; ++i) {
        rtt.measure(50ms);
    }

    auto timeout = rtt.timeout();
    // After convergence, timeout should be close to the measured value
    // (SRTT + 4*RTTVAR, but RTTVAR converges to near-zero with constant measurements)
    // With constant 50ms, SRTT→50ms, RTTVAR→0, so timeout→50ms + min_rttvar
    // But due to the algorithm, it should be well below 500ms
    REQUIRE(timeout < 500ms);
    REQUIRE(timeout >= 50ms);
}

SCENARIO("RTTEstimator increases timeout with variable measurements", "[nuclearnet][rtt]") {
    RTTEstimator rtt;

    // First establish a baseline
    for (int i = 0; i < 10; ++i) {
        rtt.measure(50ms);
    }
    auto stable_timeout = rtt.timeout();

    // Now introduce high variance
    rtt.measure(200ms);
    rtt.measure(10ms);
    rtt.measure(300ms);

    auto variable_timeout = rtt.timeout();
    // The timeout should be larger due to increased variance
    REQUIRE(variable_timeout > stable_timeout);
}

SCENARIO("RTTEstimator first measurement sets initial estimates", "[nuclearnet][rtt]") {
    RTTEstimator rtt;

    // First measurement: SRTT = R, RTTVAR = R/2, RTO = SRTT + 4*RTTVAR = R + 2R = 3R
    rtt.measure(100ms);
    auto timeout = rtt.timeout();

    // Should be approximately 3 * 100ms = 300ms
    REQUIRE(timeout >= 250ms);
    REQUIRE(timeout <= 350ms);
}
