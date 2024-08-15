#include "usage_clock.hpp"

#include <chrono>

// Windows
#if defined(_WIN32)
    #include "platform.hpp"

namespace NUClear {
namespace util {

    cpu_clock::time_point cpu_clock::now() noexcept {
        FILETIME creation_time;
        FILETIME exit_time;
        FILETIME kernel_time;
        FILETIME user_time;
        if (GetThreadTimes(GetCurrentThread(), &creation_time, &exit_time, &kernel_time, &user_time) != -1) {
            // Time in in 100 nanosecond intervals
            uint64_t time = ((uint64_t(user_time.dwHighDateTime) << 32) | user_time.dwLowDateTime)
                            + ((uint64_t(kernel_time.dwHighDateTime) << 32) | kernel_time.dwLowDateTime);
            return time_point(std::chrono::duration<uint64_t, std::ratio<1LL, 10000000LL>>(time));
        }
        return time_point();
    }

}  // namespace util
}  // namespace NUClear

#else
    #include <ctime>

namespace NUClear {
namespace util {

    cpu_clock::time_point cpu_clock::now() noexcept {
        ::timespec ts{};
        ::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
        return time_point(std::chrono::seconds(ts.tv_sec) + std::chrono::nanoseconds(ts.tv_nsec));
    }

}  // namespace util
}  // namespace NUClear

#endif  // _WIN32
