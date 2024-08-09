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

#include "Sleeper.hpp"

#include <array>
#include <chrono>

#if defined(_WIN32)

    #include <chrono>
    #include <cstdint>

    #include "platform.hpp"

namespace NUClear {
namespace util {

    // Windows requires a waitable timer to sleep for a precise amount of time
    class SleeperImpl {
    public:
        SleeperImpl() : timer(::CreateWaitableTimer(nullptr, TRUE, nullptr)) {}
        ::HANDLE timer;
    };

    Sleeper::~Sleeper() {
        ::CloseHandle(sleeper->timer);
    }

    void Sleeper::idle_sleep(const std::chrono::nanoseconds& ns) {
        ::LARGE_INTEGER ft;
        // TODO if ns is negative make it 0 as otherwise it'll become absolute time
        // Negative for relative time, positive for absolute time
        // Measures in 100ns increments so divide by 100
        ft.QuadPart = -static_cast<int64_t>(ns.count() / 100);

        ::SetWaitableTimer(impl->timer, &ft, 0, nullptr, nullptr, 0);
        ::WaitForSingleObject(impl->timer, INFINITE);
    }

}  // namespace util
}  // namespace NUClear

#else

    #include <cerrno>
    #include <cstdint>
    #include <ctime>

namespace NUClear {
namespace util {

    // No specific implementation for precise sleep on linux
    class SleeperImpl {};

    // Sleep using nanosleep on linux
    void Sleeper::idle_sleep(const std::chrono::nanoseconds& ns) {
        if (ns <= std::chrono::nanoseconds(0)) {
            return;
        }
        timespec ts{};
        ts.tv_sec  = std::chrono::duration_cast<std::chrono::seconds>(ns).count();
        ts.tv_nsec = (ns - std::chrono::seconds(ts.tv_sec)).count();

        while (::nanosleep(&ts, &ts) == -1 && errno == EINTR) {
        }
    }

}  // namespace util
}  // namespace NUClear

#endif

namespace NUClear {
namespace util {

    Sleeper::Sleeper() : sleeper(std::make_unique<SleeperImpl>()) {}

    // This must be in the .cpp file as we need the full definition of SleeperImpl
    Sleeper::~Sleeper()                    = default;
    Sleeper::Sleeper(Sleeper&&)            = default;
    Sleeper& Sleeper::operator=(Sleeper&&) = default;

    void NUClear::util::Sleeper::sleep_for(const std::chrono::nanoseconds& duration) {
        sleep_until(std::chrono::steady_clock::now() + duration);
    }

    void NUClear::util::Sleeper::sleep_until(const std::chrono::steady_clock::time_point& target) {
        using namespace std::chrono;

        for (auto start = std::chrono::steady_clock::now(); start < target; start = std::chrono::steady_clock::now()) {
            idle_sleep(target - start);
        }
    }

}  // namespace util
}  // namespace NUClear
