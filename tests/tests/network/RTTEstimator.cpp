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
#include "extension/network/RTTEstimator.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace NUClear::extension::network;
using namespace std::chrono_literals;

SCENARIO("RTTEstimator initial state", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 1.0f, 0.0f);

        THEN("the initial timeout should be 1 second") {
            REQUIRE(rtt.timeout() == 1s);
        }
    }
}

SCENARIO("RTTEstimator with constant RTT", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 0.1f, 0.0f);

        WHEN("we feed it constant RTTs of 100ms") {
            for (int i = 0; i < 20; ++i) {
                rtt.measure(100ms);
            }

            THEN("the timeout should be at least 100ms and not unreasonably high") {
                REQUIRE(rtt.timeout() >= 100ms);
                REQUIRE(rtt.timeout() <= 200ms);
            }
        }
    }
}

SCENARIO("RTTEstimator with increasing RTT", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 0.1f, 0.0f);

        WHEN("we measure 100ms then 200ms") {
            rtt.measure(100ms);
            rtt.measure(200ms);

            THEN("the timeout should be at least 200ms and not unreasonably high") {
                REQUIRE(rtt.timeout() >= 200ms);
                REQUIRE(rtt.timeout() <= 400ms);
            }
        }
    }
}

SCENARIO("RTTEstimator with decreasing RTT", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 0.2f, 0.0f);

        WHEN("we measure 200ms then 100ms") {
            rtt.measure(200ms);
            rtt.measure(100ms);

            THEN("the timeout should be at least 100ms and not unreasonably high") {
                REQUIRE(rtt.timeout() >= 100ms);
                REQUIRE(rtt.timeout() <= 400ms);
            }
        }
    }
}

SCENARIO("RTTEstimator with oscillating RTT", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 0.15f, 0.0f);

        WHEN("we feed it alternating RTTs of 100ms and 200ms") {
            for (int i = 0; i < 20; ++i) {
                rtt.measure(i % 2 == 0 ? 100ms : 200ms);
            }

            THEN("the timeout should be at least 100ms and not unreasonably high") {
                REQUIRE(rtt.timeout() >= 100ms);
                REQUIRE(rtt.timeout() <= 400ms);
            }
        }
    }
}

SCENARIO("RTTEstimator with large RTT variation", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 0.1f, 0.0f);

        WHEN("we measure 100ms then 1 second") {
            rtt.measure(100ms);
            rtt.measure(1s);

            THEN("the timeout should be at least 1s and not unreasonably high") {
                REQUIRE(rtt.timeout() >= 1s);
                REQUIRE(rtt.timeout() <= 2s);
            }
        }
    }
}

SCENARIO("RTTEstimator with small RTT variation", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 0.1f, 0.0f);

        WHEN("we measure 100ms then 110ms") {
            rtt.measure(100ms);
            rtt.measure(110ms);

            THEN("the timeout should be at least 110ms and not unreasonably high") {
                REQUIRE(rtt.timeout() >= 110ms);
                REQUIRE(rtt.timeout() <= 200ms);
            }
        }
    }
}

SCENARIO("RTTEstimator with zero RTT", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 0.001f, 0.0f);

        WHEN("we measure 0ms") {
            rtt.measure(0ms);

            THEN("the timeout should be at least 0ms and not unreasonably high") {
                REQUIRE(rtt.timeout() >= 0ms);
                REQUIRE(rtt.timeout() <= 1s);
            }
        }
    }
}

SCENARIO("RTTEstimator with very large RTT", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 30.0f, 0.0f);

        WHEN("we measure 30 seconds") {
            rtt.measure(30s);

            THEN("the timeout should be at least 30s and not unreasonably high") {
                REQUIRE(rtt.timeout() >= 30s);
                REQUIRE(rtt.timeout() <= 35s);
            }
        }
    }
}

SCENARIO("RTTEstimator exact calculation verification", "[network]") {
    GIVEN("a new RTTEstimator with known initial state") {
        // Initialize with SRTT = 100ms, RTTVAR = 50ms
        RTTEstimator rtt(0.125f, 0.25f, 0.1f, 0.05f);

        WHEN("we measure a 120ms RTT") {
            rtt.measure(120ms);

            THEN("the values should match the TCP calculation") {
                // Expected values:
                // RTTVAR = (1 - 0.25) * 50 + 0.25 * |100 - 120| = 0.75 * 50 + 0.25 * 20 = 37.5 + 5 = 42.5ms
                // SRTT = (1 - 0.125) * 100 + 0.125 * 120 = 87.5 + 15 = 102.5ms
                // RTO = 102.5 + 4 * 42.5 = 272.5ms
                REQUIRE(rtt.timeout() >= 270ms);
                REQUIRE(rtt.timeout() <= 275ms);
            }
        }
    }
}

SCENARIO("RTTEstimator spike response", "[network]") {
    GIVEN("a new RTTEstimator with stable RTT") {
        RTTEstimator rtt(0.125f, 0.25f, 0.1f, 0.0f);

        WHEN("we feed it constant 100ms RTTs then a 500ms spike") {
            // First establish a stable RTT
            for (int i = 0; i < 10; ++i) {
                rtt.measure(100ms);
            }
            auto before_spike = rtt.timeout();

            // Then inject a spike
            rtt.measure(500ms);
            auto after_spike = rtt.timeout();

            THEN("the timeout should increase but not dramatically") {
                REQUIRE(after_spike > before_spike);  // Should increase
                REQUIRE(after_spike < 1s);            // But not too much
            }

            AND_WHEN("we return to normal RTT") {
                for (int i = 0; i < 10; ++i) {
                    rtt.measure(100ms);
                }
                auto after_recovery = rtt.timeout();

                THEN("it should recover to a reasonable value") {
                    REQUIRE(after_recovery >= 100ms);
                    REQUIRE(after_recovery <= 300ms);
                }
            }
        }
    }
}

SCENARIO("RTTEstimator noise resilience", "[network]") {
    GIVEN("a new RTTEstimator") {
        RTTEstimator rtt(0.125f, 0.25f, 0.1f, 0.0f);

        WHEN("we feed it noisy RTTs around 100ms") {
            // Generate noisy RTTs: 100ms ± 20ms
            for (int i = 0; i < 50; ++i) {
                auto noise = (i % 2 == 0 ? 20ms : -20ms);
                rtt.measure(100ms + noise);
            }

            THEN("the timeout should remain stable") {
                REQUIRE(rtt.timeout() >= 100ms);
                REQUIRE(rtt.timeout() <= 300ms);
            }

            AND_WHEN("we continue with constant RTT") {
                for (int i = 0; i < 10; ++i) {
                    rtt.measure(100ms);
                }
                auto final_timeout = rtt.timeout();

                THEN("it should converge to the true RTT") {
                    REQUIRE(final_timeout >= 100ms);
                    REQUIRE(final_timeout <= 200ms);
                }
            }
        }
    }
}
