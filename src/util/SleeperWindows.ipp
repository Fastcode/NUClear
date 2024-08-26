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

#include <stdexcept>

#include "platform.hpp"

namespace NUClear {
namespace util {

    struct SleeperState {
        SleeperState() {
            // Create a waitable timer and an event to wake up the sleeper prematurely
            if (
#ifdef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
                (timer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS))
                    == NULL
                &&
#endif
                (timer = CreateWaitableTimer(NULL, TRUE, NULL)) == NULL) {
                throw std::runtime_error("Failed to create waitable timer");
            }
            waker = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        }
        ~SleeperState() {
            ::CloseHandle(timer);
            ::CloseHandle(waker);
        }
        ::HANDLE timer;
        ::HANDLE waker;
    };

    void Sleeper::sleep_until(const std::chrono::steady_clock::time_point& target) {
        auto now = std::chrono::steady_clock::now();

        if (now - target > std::chrono::nanoseconds(0)) {
            return;
        }

        std::chrono::nanoseconds ns = target - now;
        ::LARGE_INTEGER ft;
        ft.QuadPart = -static_cast<int64_t>(ns.count() / 100);

        ::SetWaitableTimer(state->timer, &ft, 0, nullptr, nullptr, 0);
        std::array<const ::HANDLE, 2> items = {state->timer, state->waker};
        ::WaitForMultipleObjects(2, items, FALSE, INFINITE);
        ::ClearEvent(state->waker);
    }

    void Sleeper::wake() {
        ::SetEvent(state->waker);
    }

}  // namespace util
}  // namespace NUClear
