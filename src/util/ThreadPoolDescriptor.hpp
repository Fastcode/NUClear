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
        /// The ID of the main thread pool (not to be confused with the ID of the main thread)
        static constexpr NUClear::id_t MAIN_THREAD_POOL_ID = 0;
        /// The ID of the default thread pool
        static constexpr NUClear::id_t DEFAULT_THREAD_POOL_ID = 1;

        /**
         * Makes the default thread pool descriptor
         */
        ThreadPoolDescriptor() noexcept : name("Default"), pool_id(ThreadPoolDescriptor::DEFAULT_THREAD_POOL_ID) {};

        ThreadPoolDescriptor(std::string name,
                             const NUClear::id_t& pool_id,
                             int thread_count     = 1,
                             bool counts_for_idle = true) noexcept
            : name(std::move(name)), pool_id(pool_id), thread_count(thread_count), counts_for_idle(counts_for_idle) {}

        /**
         * Use this descriptor when referring to all thread pools (for example when adding an idle task)
         *
         * @return The descriptor for all thread pools
         */
        static ThreadPoolDescriptor AllPools() {
            return ThreadPoolDescriptor{"All", NUClear::id_t(-1), -1, false};
        }

        /**
         * Use this descriptor when you need to refer to a thread that is not in a pool
         *
         * @return ThreadPoolDescriptor
         */
        static ThreadPoolDescriptor NonPool() {
            return ThreadPoolDescriptor{"NonPool", NUClear::id_t(-1), -1, false};
        }

        /**
         * @return The next unique ID for a new thread pool
         */
        static NUClear::id_t get_unique_pool_id() noexcept;

        /// The name of this pool
        std::string name;

        /// A unique identifier for this pool
        NUClear::id_t pool_id;

        /// The number of threads this thread pool will use
        int thread_count{0};
        /// If these threads count towards system idle
        bool counts_for_idle{true};
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_THREAD_POOL_DESCRIPTOR_HPP
