#ifndef NUCLEAR_UTIL_USAGE_CLOCK_HPP
#define NUCLEAR_UTIL_USAGE_CLOCK_HPP

#include <chrono>

namespace NUClear {
namespace util {

    /**
     * A clock that measures CPU time.
     */
    struct cpu_clock {
        using duration              = std::chrono::nanoseconds;            ///< The duration type of the clock.
        using rep                   = duration::rep;                       ///< The representation type of the duration.
        using period                = duration::period;                    ///< The tick period of the clock.
        using time_point            = std::chrono::time_point<cpu_clock>;  ///< The time point type of the clock.
        static const bool is_steady = true;                                ///< Indicates if the clock is steady.

        /**
         * Get the current time point of the cpu clock for the current thread
         *
         * @return The current time point.
         */
        static time_point now() noexcept;
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_USAGE_CLOCK_HPP
