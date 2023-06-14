/*
 * Copyright (C) 2023      Alex Biddulph <bidskii@gmail.com>
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

#ifndef NUCLEAR_UTIL_THREADPOOL_HPP
#define NUCLEAR_UTIL_THREADPOOL_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace NUClear {
namespace util {

    struct ThreadPoolIDSource {
        static constexpr uint64_t MAIN_THREAD_POOL_ID    = 0;
        static constexpr uint64_t DEFAULT_THREAD_POOL_ID = 1;

        static uint64_t get_new_pool_id() {
            static std::atomic<uint64_t> source{2};
            return source++;
        }
    };

    struct ThreadPoolDescriptor {
        /// @brief Set a unique identifier for this pool
        uint64_t pool_id{ThreadPoolIDSource::DEFAULT_THREAD_POOL_ID};

        /// @brief The number of threads this thread pool will use.
        size_t thread_count{1};
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_THREADPOOL_HPP
