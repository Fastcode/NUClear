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

#ifndef NUCLEAR_UTIL_GROUP_DESCRIPTOR_HPP
#define NUCLEAR_UTIL_GROUP_DESCRIPTOR_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "../id.hpp"

namespace NUClear {
namespace util {

    /**
     * A description of a group
     */
    struct GroupDescriptor {
        /// a unique identifier for this pool
        NUClear::id_t group_id{0};

        /// the maximum number of threads that can run concurrently in this group
        int thread_count{1};

        /**
         * Return the next unique ID for a new group
         */
        static NUClear::id_t get_unique_group_id() noexcept {
            // Make group 0 the default group
            static std::atomic<NUClear::id_t> source{1};
            return source++;
        }

        /**
         * Compare two group descriptors by their group_id to allow for sorting and uniqueness
         *
         * @param other the other group descriptor to compare to
         *
         * @return true if this group_id is less than the other group_id
         */
        bool operator<(const GroupDescriptor& other) const {
            return group_id < other.group_id;
        }
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_GROUP_DESCRIPTOR_HPP
