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

#ifndef NUCLEAR_UTIL_THREADPOOL_HPP
#define NUCLEAR_UTIL_THREADPOOL_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>

#include "../id.hpp"

namespace NUClear {
namespace util {

    /**
     * A description of a thread pool
     */
    struct ThreadPoolDescriptor {
        ThreadPoolDescriptor() = default;
        ThreadPoolDescriptor(const std::string& name,
                             const NUClear::id_t& pool_id,
                             size_t thread_count,
                             bool counts_for_idle)
            : name(name), pool_id(pool_id), thread_count(thread_count), counts_for_idle(counts_for_idle) {}

        static ThreadPoolDescriptor AllPools() {
            return ThreadPoolDescriptor{"All", NUClear::id_t(-1), size_t(-1), false};
        }

        static ThreadPoolDescriptor Invalid() {
            return ThreadPoolDescriptor{"Invalid", NUClear::id_t(-1), size_t(-1), false};
        }

        /// the name of this pool
        std::string name = "Default";

        /// a unique identifier for this pool
        NUClear::id_t pool_id{ThreadPoolDescriptor::DEFAULT_THREAD_POOL_ID};

        /// the number of threads this thread pool will use
        size_t thread_count{0};
        /// if these threads count towards system idle
        bool counts_for_idle{true};

        /// the ID of the main thread pool (not to be confused with the ID of the main thread)
        static const NUClear::id_t MAIN_THREAD_POOL_ID;
        /// the ID of the default thread pool
        static const NUClear::id_t DEFAULT_THREAD_POOL_ID;

        /**
         * Return the next unique ID for a new thread pool
         */
        static NUClear::id_t get_unique_pool_id() noexcept {
            static std::atomic<NUClear::id_t> source{2};
            return source++;
        }
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_THREADPOOL_HPP
