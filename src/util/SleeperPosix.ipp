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

#include <csignal>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>

#include "Sleeper.hpp"

extern "C" {
static void signal_handler(int) {
    // Do nothing, we just want to interrupt the sleep
}
}

namespace NUClear {
namespace util {

    struct SleeperState {
        /// If the sleeper has been interrupted
        std::atomic<bool> interrupted{false};
        /// The thread that is currently sleeping
        pthread_t sleeping_thread{};
    };


    Sleeper::Sleeper() : state(std::make_unique<SleeperState>()) {
        struct sigaction act {};
        // If the signal is default handler, replace it with an ignore handler
        if (!::sigaction(SIGUSR1, nullptr, &act) && act.sa_handler == SIG_DFL) {
            act.sa_handler = signal_handler;
            ::sigaction(SIGUSR1, &act, nullptr);
        }
    }
    Sleeper::~Sleeper() = default;

    void Sleeper::wake() {
        state->interrupted.store(true, std::memory_order_release);
        if (state->sleeping_thread != pthread_t{}) {
            pthread_kill(state->sleeping_thread, SIGUSR1);
        }
    }

    void Sleeper::sleep_until(const std::chrono::steady_clock::time_point& target) {
        if (state->sleeping_thread != pthread_t{}) {
            throw std::logic_error("Sleeper object cannot be used to sleep multiple times");
        }

        // Store the current sleeping thread so we can wake it up if we need to
        state->sleeping_thread = ::pthread_self();

        while (!state->interrupted.load(std::memory_order_acquire)) {
            auto now = std::chrono::steady_clock::now();
            if (now >= target) {
                break;
            }

            // Sleep for the remaining time
            std::chrono::nanoseconds ns = target - now;
            timespec ts{};
            ts.tv_sec  = std::chrono::duration_cast<std::chrono::seconds>(ns).count();
            ts.tv_nsec = (ns - std::chrono::seconds(ts.tv_sec)).count();
            ::nanosleep(&ts, &ts);
        }

        // Not interrupted as we are leaving so if we are interrupted before entering we immediately skip
        state->sleeping_thread = pthread_t{};
        state->interrupted.store(false, std::memory_order_release);
    }

}  // namespace util
}  // namespace NUClear
