#ifndef NUCLEAR_UTIL_USAGE_CLOCK_HPP
#define NUCLEAR_UTIL_USAGE_CLOCK_HPP

#include <chrono>

namespace NUClear {
namespace util {

    struct user_cpu_clock {
        using duration              = std::chrono::nanoseconds;
        using rep                   = duration::rep;
        using period                = duration::period;
        using time_point            = std::chrono::time_point<user_cpu_clock>;
        static const bool is_steady = true;

        static time_point now() noexcept;
    };

    struct kernel_cpu_clock {
        using duration              = std::chrono::nanoseconds;
        using rep                   = duration::rep;
        using period                = duration::period;
        using time_point            = std::chrono::time_point<kernel_cpu_clock>;
        static const bool is_steady = true;

        static time_point now() noexcept;
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_USAGE_CLOCK_HPP
