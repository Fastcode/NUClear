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

namespace NUClear {
namespace util {


    namespace {
#ifndef _WIN32

    #include <pthread.h>

        /// The minimum priority level for the SCHED_RR policy
        static const int min_rr_priority = sched_get_priority_min(SCHED_RR);
        /// The maximum priority level for the SCHED_RR policy
        static const int max_rr_priority = sched_get_priority_max(SCHED_RR);
        /// The maximum priority level for the SCHED_FIFO policy
        static const int max_fifo_priority = sched_get_priority_max(SCHED_FIFO);

        void set_priority(const PriorityLevel& priority) {

            sched_param param{};
            switch (priority) {
                case PriorityLevel::IDLE: {  // Use the idle scheduler if it exists
    #ifdef SHED_IDLE
                    pthread_setschedparam(pthread_self(), SCHED_IDLE, &param);
    #else
                    pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
    #endif
                } break;
                case PriorityLevel::LOW:
                case PriorityLevel::NORMAL: {  // Uses the default scheduler and lets NUClear order tasks
                    pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
                } break;
                case PriorityLevel::HIGH: {
                    // The round-robin scheduler gives time slices to each thread and will preempt any thread with a
                    // lower priority
                    param.sched_priority = (min_rr_priority + max_rr_priority) / 2;
                    pthread_setschedparam(pthread_self(), SCHED_RR, &param);
                } break;
                case PriorityLevel::REALTIME: {
                    // The FIFO scheduler will run the thread until it yields or is preempted by a higher priority
                    // thread
                    param.sched_priority = max_fifo_priority;
                    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
                } break;
                default: {
                    throw std::runtime_error("Unknown priority level");
                }
            }
        }  // namespace

#else

    #include "platform.hpp"

        void set_priority(const PriorityLevel& priority) {

            switch (priority) {
                case IDLE: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE); break;
                case LOW: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST); break;
                default:
                case NORMAL: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL); break;
                case HIGH: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); break;
                case REALTIME: SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL); break;
            }
        }

#endif  // def _WIN32
    }  // namespace


    ThreadPriority::ThreadPriority(const PriorityLevel& priority) : previous_priority(current_priority) {
        current_priority = priority;
        set_priority(priority);
    }

    ThreadPriority::~ThreadPriority() {
        set_priority(previous_priority);
    }

    ATTRIBUTE_TLS PriorityLevel ThreadPriority::current_priority = PriorityLevel::NORMAL;

}  // namespace util
}  // namespace NUClear
