#include "precise_sleep.hpp"

namespace NUClear {
namespace util {

#if defined(_WIN32)

    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>

    #include <chrono>
    #include <cstdint>

    int64_t get_performance_frequency() {
        ::LARGE_INTEGER freq;
        ::QueryPerformanceFrequency(&freq);
        return freq.QuadPart;
    }

    int64_t performance_frequency() {
        static int64_t freq = get_performance_frequency();
        return freq;
    }

    int64_t performance_counter() {
        ::LARGE_INTEGER counter;
        ::QueryPerformanceCounter(&counter);
        return counter.QuadPart;
    }

    void precise_sleep(const std::chrono::nanoseconds& ns) {
        ::LARGE_INTEGER ft;
        // TODO if ns is negative make it 0 as otherwise it'll become absolute time
        // Negative for relative time, positive for absolute time
        // Measures in 100ns increments so divide by 100
        ft.QuadPart = -static_cast<int64_t>(ns.count / 100);

        ::HANDLE timer = ::CreateWaitableTimer(nullptr, TRUE, nullptr);
        ::SetWaitableTimer(timer, &ft, 0, nullptr, nullptr, 0);
        ::WaitForSingleObject(timer, INFINITE);
        ::CloseHandle(timer);
    }

#else

    #include <cerrno>
    #include <cstdint>
    #include <ctime>

    void precise_sleep(const std::chrono::nanoseconds& ns) {
        struct timespec ts;
        ts.tv_sec  = std::chrono::duration_cast<std::chrono::seconds>(ns).count();
        ts.tv_nsec = (ns - std::chrono::seconds(ts.tv_sec)).count();

        while (::nanosleep(&ts, &ts) == -1 && errno == EINTR) {
        }
    }

#endif

}  // namespace util
}  // namespace NUClear
