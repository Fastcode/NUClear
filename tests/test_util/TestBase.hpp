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

#ifndef TEST_UTIL_TEST_BASE_HPP
#define TEST_UTIL_TEST_BASE_HPP

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <utility>

#include "nuclear"
#include "test_util/diff_string.hpp"

namespace test_util {

template <typename BaseClass>
class TestBase : public NUClear::Reactor {
public:
    /**
     * Struct to use to emit each step of the test.
     *
     * By doing each step in a separate reaction with low priority it will ensure that everything has finished changing
     * before the next step is run
     *
     * @tparam i the number of the step
     */
    template <int i>
    struct Step {};

    /**
     * Emit this struct to fail the test
     */
    struct Fail {
        explicit Fail(std::string message) : message(std::move(message)) {}
        std::string message;
    };

    explicit TestBase(std::unique_ptr<NUClear::Environment> environment,
                      const bool& shutdown_on_idle             = true,
                      const std::chrono::milliseconds& timeout = std::chrono::milliseconds(1000))
        : Reactor(std::move(environment)) {

        // Shutdown if the system is idle
        if (shutdown_on_idle) {
            on<Idle<>>().then([this] { powerplant.shutdown(); });
        }

        on<Shutdown>().then([this] {
            const std::lock_guard<std::mutex> lock(timeout_mutex);
            clean_shutdown = true;
            timeout_cv.notify_all();
        });

        // Timeout if the test doesn't complete in time
        // Typically we would use a watchdog, however it is subject to Time Travel
        // So instead spawn a thread that will wait for the timeout and then fail the test and shut down
        on<Always>().then("Test Timeout", [this, timeout] {
            if (clean_shutdown) {
                return;
            }
            std::unique_lock<std::mutex> lock(timeout_mutex);
            timeout_cv.wait_for(lock, timeout);

            if (!clean_shutdown) {
                powerplant.shutdown(true);
                emit<Scope::INLINE>(
                    std::make_unique<Fail>("Test timed out after " + std::to_string(timeout.count()) + " ms"));
            }
        });

        on<Trigger<Fail>, MainThread>().then([this](const Fail& f) {
            INFO(f.message);
            CHECK(false);
        });
    }

private:
    std::mutex timeout_mutex;
    std::condition_variable timeout_cv;
    bool clean_shutdown = false;
};

}  // namespace test_util

#endif  // TEST_UTIL_TEST_BASE_HPP
