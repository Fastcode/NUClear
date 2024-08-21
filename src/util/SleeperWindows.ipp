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

#include "platform.hpp"

namespace NUClear {
namespace util {

    struct SleeperState {
        SleeperState() {
            // Create a waitable timer and an event to wake up the sleeper prematurely
            timer = ::CreateWaitableTimer(nullptr, TRUE, nullptr);
            waker = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
        }
        ~SleeperState() {
            ::CloseHandle(timer);
            ::CloseHandle(waker);
        }
        ::HANDLE timer;
        ::HANDLE waker;
    };

    void Sleeper::sleep_until(const std::chrono::steady_clock& target) {
        auto now = std::chrono::steady_clock::now();

        if (now - target > std::chrono::nanoseconds(0)) {
            return;
        }

        auto ns = target - now;
        ::LARGE_INTEGER ft;
        // TODO if ns is negative make it 0 as otherwise it'll become absolute time
        // Negative for relative time, positive for absolute time
        // Measures in 100ns increments so divide by 100
        ft.QuadPart = -static_cast<int64_t>(ns.count() / 100);

        ::SetWaitableTimer(state->timer, &ft, 0, nullptr, nullptr, 0);
        std::array<const ::HANDLE, 2> items = {state->timer, state->waker};
        ::WaitForMultipleObjects(2, items, FALSE, INFINITE);
    }

}  // namespace util
}  // namespace NUClear
