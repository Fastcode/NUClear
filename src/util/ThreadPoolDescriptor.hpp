/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_THREAD_POOL_DESCRIPTOR_HPP
#define NUCLEAR_UTIL_THREAD_POOL_DESCRIPTOR_HPP

#include <atomic>
#include <cstddef>
#include <string>

#include "../id.hpp"

namespace NUClear {
namespace util {

    /**
     * A description of a thread pool
     */
    struct ThreadPoolDescriptor {

        ThreadPoolDescriptor(std::string name,
                             const int& thread_count            = 1,
                             const bool& counts_for_idle        = true,
                             const bool& tasks_must_run_on_pool = false,
                             const bool& continue_on_shutdown   = false) noexcept
            : name(std::move(name))
            , thread_count(thread_count)
            , counts_for_idle(counts_for_idle)
            , tasks_must_run_on_pool(tasks_must_run_on_pool)
            , continue_on_shutdown(continue_on_shutdown) {}

        /// The name of this pool
        std::string name;
        /// The number of threads this thread pool will use
        int thread_count;
        /// If these threads count towards system idle
        bool counts_for_idle;
        /// If tasks which are bound to this pool must be run on this pool (no Direct on their own thread)
        bool tasks_must_run_on_pool;
        /// If this thread pool will continue to accept tasks after shutdown and only stop on scheduler destruction
        bool continue_on_shutdown;
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_THREAD_POOL_DESCRIPTOR_HPP
