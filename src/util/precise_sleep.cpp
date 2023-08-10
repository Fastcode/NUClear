#include "precise_sleep.hpp"

#if defined(_WIN32)

    #include <chrono>
    #include <cstdint>

    #include "platform.hpp"

namespace NUClear {
namespace util {

    void precise_sleep(const std::chrono::nanoseconds& ns) {
        ::LARGE_INTEGER ft;
        // TODO if ns is negative make it 0 as otherwise it'll become absolute time
        // Negative for relative time, positive for absolute time
        // Measures in 100ns increments so divide by 100
        ft.QuadPart = -static_cast<int64_t>(ns.count() / 100);

        ::HANDLE timer = ::CreateWaitableTimer(nullptr, TRUE, nullptr);
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
