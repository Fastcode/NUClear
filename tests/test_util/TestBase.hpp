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

#ifndef TEST_UTIL_TESTBASE_HPP
#define TEST_UTIL_TESTBASE_HPP

#include <catch.hpp>
#include <string>
#include <utility>

#include "nuclear"
#include "test_util/diff_string.hpp"

namespace test_util {

template <typename BaseClass, int timeout = 1000>
class TestBase : public NUClear::Reactor {
public:
    /**
     * @brief Struct to use to emit each step of the test, by doing each step in a separate reaction with low priority,
     * it will ensure that everything has finished changing before the next step is run
     *
     * @tparam i the number of the step
     */
    template <int i>
    struct Step {};

    /**
     * @brief Struct to handle shutting down the powerplant when the system is idle (i.e. the unit test(s) are finished)
     */
    struct ShutdownOnIdle {};

    explicit TestBase(std::unique_ptr<NUClear::Environment> environment, const bool& shutdown_on_idle = true)
        : Reactor(std::move(environment)) {

        // Shutdown if the system is idle
        on<Trigger<ShutdownOnIdle>, Priority::IDLE>().then([this] { powerplant.shutdown(); });
        on<Startup>().then([this, shutdown_on_idle] {
            if (shutdown_on_idle) {
                emit(std::make_unique<ShutdownOnIdle>());
            }
        });

        // Timeout if the test doesn't complete in time
        on<Watchdog<BaseClass, timeout, std::chrono::milliseconds>, MainThread>().then([this] {
            powerplant.shutdown();
            INFO("Test timed out");
            CHECK(false);
        });
    }
};

}  // namespace test_util

#endif  // TEST_UTIL_TESTBASE_HPP
