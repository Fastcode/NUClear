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

#include "precise_sleep.hpp"

#if defined(_WIN32)

    #include <chrono>
    #include <cstdint>
    #include <stdexcept>

    #include "platform.hpp"

namespace NUClear {
namespace util {

    void precise_sleep(const std::chrono::nanoseconds& ns) {
        ::LARGE_INTEGER ft;
        // TODO if ns is negative make it 0 as otherwise it'll become absolute time
        // Negative for relative time, positive for absolute time
        // Measures in 100ns increments so divide by 100
        ft.QuadPart = -static_cast<int64_t>(ns.count() / 100);
        // Create a waitable timer with as high resolution as possible
        ::HANDLE timer{};
        if (
    #ifdef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
            (timer = CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS)) == NULL
            &&
    #endif
            (timer = CreateWaitableTimer(NULL, TRUE, NULL)) == NULL) {
            throw std::runtime_error("Failed to create waitable timer");
        }

        ::SetWaitableTimer(timer, &ft, 0, nullptr, nullptr, 0);
        ::WaitForSingleObject(timer, INFINITE);
        ::CloseHandle(timer);
    }

}  // namespace util
}  // namespace NUClear

#else

    #include <cerrno>
    #include <cstdint>
    #include <ctime>

namespace NUClear {
namespace util {

    void precise_sleep(const std::chrono::nanoseconds& ns) {
        timespec ts{};
        ts.tv_sec  = std::chrono::duration_cast<std::chrono::seconds>(ns).count();
        ts.tv_nsec = (ns - std::chrono::seconds(ts.tv_sec)).count();

        while (::nanosleep(&ts, &ts) == -1 && errno == EINTR) {
        }
    }

}  // namespace util
}  // namespace NUClear

#endif
