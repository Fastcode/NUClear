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

#ifndef NUCLEAR_UTIL_UPDATE_CURRENT_THREAD_PRIORITY_HPP
#define NUCLEAR_UTIL_UPDATE_CURRENT_THREAD_PRIORITY_HPP

#include "../threading/scheduler/queue/Priority.hpp"

#ifndef _WIN32

    #include <pthread.h>

inline void update_current_thread_priority(const NUClear::threading::PriorityLevel priority) {

    const auto min_priority = sched_get_priority_min(SCHED_RR);
    const auto max_priority = sched_get_priority_max(SCHED_RR);
    const auto range        = max_priority - min_priority;

    int sched_priority = min_priority;
    switch (priority) {
        case NUClear::threading::PriorityLevel::REALTIME: sched_priority = min_priority + range; break;
        case NUClear::threading::PriorityLevel::HIGH: sched_priority = min_priority + (range * 4) / 5; break;
        case NUClear::threading::PriorityLevel::NORMAL: sched_priority = min_priority + (range * 3) / 5; break;
        case NUClear::threading::PriorityLevel::LOW: sched_priority = min_priority + (range * 2) / 5; break;
        case NUClear::threading::PriorityLevel::IDLE: sched_priority = min_priority + range / 5; break;
    }

    sched_param p{};
    p.sched_priority = sched_priority;
    pthread_setschedparam(pthread_self(), SCHED_RR, &p);
}
#endif  // ndef _WIN32

#ifdef _WIN32

    #include "platform.hpp"

inline void update_current_thread_priority(const NUClear::threading::PriorityLevel priority) {

    switch (priority) {
        case NUClear::threading::PriorityLevel::REALTIME: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        } break;
        case NUClear::threading::PriorityLevel::HIGH: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        } break;
        case NUClear::threading::PriorityLevel::NORMAL: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
        } break;
        case NUClear::threading::PriorityLevel::LOW: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
        } break;
        case NUClear::threading::PriorityLevel::IDLE: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
        } break;
    }
}

#endif  // def _WIN32

#endif  // NUCLEAR_UTIL_UPDATE_CURRENT_THREAD_PRIORITY_HPP
