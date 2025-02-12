/*
 * MIT License
 *
 * Copyright (c) 2025 NUClear Contributors
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

#include "ThreadPriority.hpp"

#include "../PriorityLevel.hpp"

#ifdef _WIN32
    #include "platform.hpp"

namespace NUClear {
namespace util {

    void set_current_thread_priority(const PriorityLevel& priority) noexcept {

        switch (priority) {
            case IDLE: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE); break;
            case LOWEST: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST); break;
            case LOW: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL); break;
            default:  // Default to normal if someone broke the enum
            case NORMAL: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL); break;
            case HIGH: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL); break;
            case HIGHEST: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); break;
            case REALTIME: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL); break;
        }
    }


    PriorityLevel get_current_thread_priority() noexcept {
        int priority = GetThreadPriority(GetCurrentThread());
        switch (priority) {
            case THREAD_PRIORITY_IDLE: return PriorityLevel::IDLE;
            case THREAD_PRIORITY_LOWEST: return PriorityLevel::LOWEST;
            case THREAD_PRIORITY_BELOW_NORMAL: return PriorityLevel::LOW;
            case THREAD_PRIORITY_NORMAL: return PriorityLevel::NORMAL;
            case THREAD_PRIORITY_ABOVE_NORMAL: return PriorityLevel::HIGH;
            case THREAD_PRIORITY_HIGHEST: return PriorityLevel::HIGHEST;
            case THREAD_PRIORITY_TIME_CRITICAL: return PriorityLevel::REALTIME;
            default: return PriorityLevel::NORMAL;
        }
    }

}  // namespace util
}  // namespace NUClear

#elif defined(__linux__)
    #include <pthread.h>
    #include <sched.h>

namespace NUClear {
namespace util {

    namespace {
        /// The minimum priority level for the SCHED_RR policy
        const int min_rr_priority = sched_get_priority_min(SCHED_RR);
        /// The maximum priority level for the SCHED_RR policy
        const int max_rr_priority = sched_get_priority_max(SCHED_RR);
        /// The step unit to use for the SCHED_RR policy
        const int step_rr_priority = (max_rr_priority - min_rr_priority) / 6;
        /// The minimum priority level for the SCHED_FIFO policy
        const int min_fifo_priority = sched_get_priority_min(SCHED_FIFO);
        /// The maximum priority level for the SCHED_FIFO policy
        const int max_fifo_priority = sched_get_priority_max(SCHED_FIFO);
        /// The step unit to use for the SCHED_FIFO policy
        const int step_fifo_priority = (max_fifo_priority - min_fifo_priority) / 6;
    }  // namespace

    PriorityLevel get_current_thread_priority() noexcept {
        int policy{};
        sched_param param{};
        pthread_getschedparam(pthread_self(), &policy, &param);

        switch (policy) {
            default: return PriorityLevel::UNKNOWN;
            case SCHED_RR:
                if (param.sched_priority == min_rr_priority + 0 * step_rr_priority) {
                    return PriorityLevel::IDLE;
                }
                if (param.sched_priority == min_rr_priority + 1 * step_rr_priority) {
                    return PriorityLevel::LOWEST;
                }
                if (param.sched_priority == min_rr_priority + 2 * step_rr_priority) {
                    return PriorityLevel::LOW;
                }
                if (param.sched_priority == min_rr_priority + 3 * step_rr_priority) {
                    return PriorityLevel::NORMAL;
                }
                if (param.sched_priority == min_rr_priority + 4 * step_rr_priority) {
                    return PriorityLevel::HIGH;
                }
                if (param.sched_priority == min_rr_priority + 5 * step_rr_priority) {
                    return PriorityLevel::HIGHEST;
                }
                break;
            case SCHED_FIFO:
                if (param.sched_priority == min_fifo_priority + 6 * step_fifo_priority)
                    return PriorityLevel::REALTIME;
                break;
        }

        return PriorityLevel::UNKNOWN;
    }

    void set_current_thread_priority(const PriorityLevel& priority) noexcept {
        sched_param param{};
        switch (priority) {
            case PriorityLevel::IDLE:
                param.sched_priority = min_rr_priority + 0 * step_rr_priority;
                pthread_setschedparam(pthread_self(), SCHED_RR, &param);
                break;
            case PriorityLevel::LOWEST:
                param.sched_priority = min_rr_priority + 1 * step_rr_priority;
                pthread_setschedparam(pthread_self(), SCHED_RR, &param);
                break;
            case PriorityLevel::LOW:
                param.sched_priority = min_rr_priority + 2 * step_rr_priority;
                pthread_setschedparam(pthread_self(), SCHED_RR, &param);
                break;
            default:  // Default to normal if someone broke the enum
            case PriorityLevel::NORMAL:
                param.sched_priority = min_rr_priority + 3 * step_rr_priority;
                pthread_setschedparam(pthread_self(), SCHED_RR, &param);
                break;
            case PriorityLevel::HIGH:
                param.sched_priority = min_rr_priority + 4 * step_rr_priority;
                pthread_setschedparam(pthread_self(), SCHED_RR, &param);
                break;
            case PriorityLevel::HIGHEST:
                param.sched_priority = min_rr_priority + 5 * step_rr_priority;
                pthread_setschedparam(pthread_self(), SCHED_RR, &param);
                break;
            case PriorityLevel::REALTIME:
                param.sched_priority = min_fifo_priority + 6 * step_fifo_priority;
                pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
                break;
        }
    }

}  // namespace util
}  // namespace NUClear

#elif defined(__APPLE__)

    #include <pthread.h>

namespace NUClear {
namespace util {

    PriorityLevel get_current_thread_priority() noexcept {
        qos_class_t qos{};
        int relative_priority{};
        pthread_get_qos_class_np(pthread_self(), &qos, &relative_priority);

        return qos == QOS_CLASS_BACKGROUND && relative_priority == 0         ? PriorityLevel::IDLE
               : qos == QOS_CLASS_UTILITY && relative_priority == 0          ? PriorityLevel::LOWEST
               : qos == QOS_CLASS_UTILITY && relative_priority == -1         ? PriorityLevel::LOW
               : qos == QOS_CLASS_DEFAULT && relative_priority == 0          ? PriorityLevel::NORMAL
               : qos == QOS_CLASS_USER_INITIATED && relative_priority == 0   ? PriorityLevel::HIGH
               : qos == QOS_CLASS_USER_INITIATED && relative_priority == -1  ? PriorityLevel::HIGHEST
               : qos == QOS_CLASS_USER_INTERACTIVE && relative_priority == 0 ? PriorityLevel::REALTIME
                                                                             : PriorityLevel::UNKNOWN;
    }

    void set_current_thread_priority(const PriorityLevel& priority) noexcept {
        switch (priority) {
            case PriorityLevel::IDLE: pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, 0); break;
            case PriorityLevel::LOWEST: pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0); break;
            case PriorityLevel::LOW: pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, -1); break;
            default:  // Default to normal if someone broke the enum
            case PriorityLevel::NORMAL: pthread_set_qos_class_self_np(QOS_CLASS_DEFAULT, 0); break;
            case PriorityLevel::HIGH: pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0); break;
            case PriorityLevel::HIGHEST: pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, -1); break;
            case PriorityLevel::REALTIME: pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0); break;
        }
    }

}  // namespace util
}  // namespace NUClear

#else
    #error "Unsupported platform"
#endif

namespace NUClear {
namespace util {

    ThreadPriority::ThreadPriority(const PriorityLevel& priority) : previous_priority(current_priority) {
        if (priority != current_priority) {
            current_priority = priority;
            set_current_thread_priority(current_priority);
        }
    }

    ThreadPriority::~ThreadPriority() noexcept {
        if (current_priority != previous_priority) {
            current_priority = previous_priority;
            set_current_thread_priority(current_priority);
        }
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local PriorityLevel ThreadPriority::current_priority = get_current_thread_priority();

}  // namespace util
}  // namespace NUClear
