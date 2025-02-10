/*
 * MIT License
 *
 * Copyright (c) 2016 NUClear Contributors
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

namespace {
void set_priority(const NUClear::PriorityLevel& priority) {

    switch (priority) {
        case IDLE: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE); break;
        case LOWEST: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST); break;
        case LOW: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL); break;
        case NORMAL: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL); break;
        case HIGH: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL); break;
        case HIGHEST: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); break;
        case REALTIME: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL); break;
    }
}
}  // namespace

#elif defined(__linux__)

    #include <pthread.h>
    #include <sched.h>

namespace {

/// The minimum priority level for the SCHED_RR policy
const int min_rr_priority = sched_get_priority_min(SCHED_RR);
/// The maximum priority level for the SCHED_RR policy
const int max_rr_priority = sched_get_priority_max(SCHED_RR);
/// The maximum priority level for the SCHED_FIFO policy
const int max_fifo_priority = sched_get_priority_max(SCHED_FIFO);

void set_priority(const NUClear::PriorityLevel& priority) {

    sched_param param{};
    switch (priority) {
        case NUClear::PriorityLevel::IDLE: pthread_setschedparam(pthread_self(), SCHED_IDLE, &param); break;
        case NUClear::PriorityLevel::LOWEST:
        case NUClear::PriorityLevel::LOW: pthread_setschedparam(pthread_self(), SCHED_OTHER, &param); break;
        default:  // Default to normal if someone broke the enum
        case NUClear::PriorityLevel::NORMAL:
            param.sched_priority = min_rr_priority;
            pthread_setschedparam(pthread_self(), SCHED_RR, &param);  // Min realtime beats all non-realtime
            break;
        case NUClear::PriorityLevel::HIGH:
            param.sched_priority = (min_rr_priority + max_rr_priority + 1) / 2;  // Halfway between min and max
            pthread_setschedparam(pthread_self(), SCHED_RR, &param);
            break;
        case NUClear::PriorityLevel::HIGHEST:
            param.sched_priority = max_rr_priority - 1;  // One less than max so it can be preempted by realtime
            pthread_setschedparam(pthread_self(), SCHED_RR, &param);
            break;
        case NUClear::PriorityLevel::REALTIME:
            param.sched_priority = max_fifo_priority;  // Max priority for SCHED_FIFO
            pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
            break;
    }
}

}  // namespace

#elif defined(__APPLE__)

    #include <pthread.h>

namespace {

/// The minimum priority level for the SCHED_RR policy
const int min_rr_priority = sched_get_priority_min(SCHED_RR);
/// The maximum priority level for the SCHED_RR policy
const int max_rr_priority = sched_get_priority_max(SCHED_RR);

void set_priority(const NUClear::PriorityLevel& priority) {
    auto sched_priority = min_rr_priority + (priority / (max_rr_priority - min_rr_priority));

    sched_param p{};
    p.sched_priority = sched_priority;
    pthread_setschedparam(pthread_self(), SCHED_RR, &p);

    // TODO this fails in CI maybe the system calls are too much?
    // switch (priority) {
    //     case NUClear::PriorityLevel::IDLE: pthread_set_qos_class_self_np(QOS_CLASS_BACKGROUND, 0); break;
    //     case NUClear::PriorityLevel::LOWEST: pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 1); break;
    //     case NUClear::PriorityLevel::LOW: pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0); break;
    //     case NUClear::PriorityLevel::NORMAL: pthread_set_qos_class_self_np(QOS_CLASS_DEFAULT, 0); break;
    //     case NUClear::PriorityLevel::HIGH: pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 1); break;
    //     case NUClear::PriorityLevel::HIGHEST: pthread_set_qos_class_self_np(QOS_CLASS_USER_INITIATED, 0); break;
    //     case NUClear::PriorityLevel::REALTIME: pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0); break;
    // }
}

}  // namespace

#else
    #error "Unsupported platform"
#endif

namespace NUClear {
namespace util {

    ThreadPriority::ThreadPriority(const NUClear::PriorityLevel& priority) : previous_priority(current_priority) {
        if (previous_priority != current_priority) {
            current_priority = priority;
            ::set_priority(current_priority);
        }
    }

    ThreadPriority::~ThreadPriority() noexcept {
        if (current_priority != previous_priority) {
            current_priority = previous_priority;
            ::set_priority(current_priority);
        }
    }

    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    thread_local NUClear::PriorityLevel ThreadPriority::current_priority = NUClear::PriorityLevel::NORMAL;

}  // namespace util
}  // namespace NUClear
