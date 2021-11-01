/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#ifndef _WIN32

#    include <pthread.h>

inline void update_current_thread_priority(int priority) {

    // TODO SCHED_NORMAL for normal threads
    // TODO SCHED_FIFO for realtime threads
    // TODO SCHED_RR for high priority threads

    auto sched_priority = sched_get_priority_min(SCHED_RR)
                          + (priority / (sched_get_priority_max(SCHED_RR) - sched_get_priority_min(SCHED_RR)));

    sched_param p;
    p.sched_priority = sched_priority;
    pthread_setschedparam(pthread_self(), SCHED_RR, &p);
}
#endif  // ndef _WIN32

#ifdef _WIN32

#    include "platform.hpp"

inline void update_current_thread_priority(int priority) {

    switch ((priority * 7) / 1000) {
        case 0: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
        } break;
        case 1: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
        } break;
        case 2: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
        } break;
        case 3: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
        } break;
        case 4: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
        } break;
        case 5: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
        } break;
        case 6: {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        } break;
    }
}

#endif  // def _WIN32

#endif  // NUCLEAR_UTIL_UPDATE_CURRENT_THREAD_PRIORITY_HPP
