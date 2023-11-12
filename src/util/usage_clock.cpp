#include "usage_clock.hpp"


// Linux
#if defined(__linux__)
    #include <sys/resource.h>

namespace NUClear {
namespace util {

    user_cpu_clock::time_point user_cpu_clock::now() noexcept {
        struct rusage usage;
        int err = ::getrusage(RUSAGE_THREAD, &usage);
        if (err == 0) {
            return time_point(std::chrono::seconds(usage.ru_utime.tv_sec)
                              + std::chrono::microseconds(usage.ru_utime.tv_usec));
        }
        return time_point();
    }

    kernel_cpu_clock::time_point kernel_cpu_clock::now() noexcept {
        struct rusage usage;
        int err = ::getrusage(RUSAGE_THREAD, &usage);
        if (err == 0) {
            return time_point(std::chrono::seconds(usage.ru_stime.tv_sec)
                              + std::chrono::microseconds(usage.ru_stime.tv_usec));
        }
        return time_point();
    }

}  // namespace util
}  // namespace NUClear

// Mac OS X
#elif defined(__MACH__) && defined(__APPLE__)

    #include <errno.h>
    #include <mach/mach.h>
    #include <sys/resource.h>

namespace NUClear {
namespace util {

    user_cpu_clock::time_point user_cpu_clock::now() noexcept {
        thread_basic_info_data_t info{};
        mach_msg_type_number_t info_count = THREAD_BASIC_INFO_COUNT;
        kern_return_t kern_err;

        mach_port_t port = mach_thread_self();
        kern_err         = thread_info(port, THREAD_BASIC_INFO, reinterpret_cast<thread_info_t>(&info), &info_count);
        mach_port_deallocate(mach_task_self(), port);

        if (kern_err == KERN_SUCCESS) {
            return time_point(std::chrono::seconds(info.user_time.seconds)
                              + std::chrono::microseconds(info.user_time.microseconds));
        }
        return time_point();
    }

    kernel_cpu_clock::time_point kernel_cpu_clock::now() noexcept {
        thread_basic_info_data_t info{};
        mach_msg_type_number_t info_count = THREAD_BASIC_INFO_COUNT;
        kern_return_t kern_err;

        mach_port_t port = mach_thread_self();
        kern_err         = thread_info(port, THREAD_BASIC_INFO, reinterpret_cast<thread_info_t>(&info), &info_count);
        mach_port_deallocate(mach_task_self(), port);

        if (kern_err == KERN_SUCCESS) {
            return time_point(std::chrono::seconds(info.system_time.seconds)
                              + std::chrono::microseconds(info.system_time.microseconds));
        }
        return time_point();
    }

}  // namespace util
}  // namespace NUClear

// Windows
#elif defined(_WIN32)
    #include "platform.hpp"

namespace NUClear {
namespace util {

    user_cpu_clock::time_point user_cpu_clock::now() noexcept {
        FILETIME creation_time;
        FILETIME exit_time;
        FILETIME kernel_time;
        FILETIME user_time;
        if (GetThreadTimes(GetCurrentThread(), &creation_time, &exit_time, &kernel_time, &user_time) != -1) {
            // Time in in 100 nanosecond intervals
            uint64_t time = (uint64_t(user_time.dwHighDateTime) << 32) | user_time.dwLowDateTime;
            return time_point(std::chrono::duration<uint64_t, std::ratio<1LL, 10000000LL>>(time));
        }
        return time_point();
    }

    kernel_cpu_clock::time_point kernel_cpu_clock::now() noexcept {
        FILETIME creation_time;
        FILETIME exit_time;
        FILETIME kernel_time;
        FILETIME user_time;
        if (GetThreadTimes(GetCurrentThread(), &creation_time, &exit_time, &kernel_time, &user_time) != -1) {
            // Time in in 100 nanosecond intervals
            uint64_t time = (uint64_t(kernel_time.dwHighDateTime) << 32) | kernel_time.dwLowDateTime;
            return time_point(std::chrono::duration<uint64_t, std::ratio<1LL, 10000000LL>>(time));
        }
        return time_point();
    }

}  // namespace util
}  // namespace NUClear

#endif  // OS
