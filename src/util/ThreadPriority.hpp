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

#ifndef NUCLEAR_UTIL_THREAD_PRIORITY_HPP
#define NUCLEAR_UTIL_THREAD_PRIORITY_HPP

#include "../PriorityLevel.hpp"

namespace NUClear {
namespace util {

    /**
     * An RAII class to lock the current thread to a specific priority level and then restore it when the lock goes out
     * of scope.
     */
    class ThreadPriority {
    public:
        /**
         * Lock the current thread to a specific priority level
         *
         * @param priority the priority level to lock the current thread to
         */
        ThreadPriority(const PriorityLevel& priority);

        // No copying or moving so this stays strictly stack based
        ThreadPriority(const ThreadPriority&)             = delete;
        ThreadPriority& operator=(const ThreadPriority&)  = delete;
        ThreadPriority(ThreadPriority&& other)            = delete;
        ThreadPriority& operator=(ThreadPriority&& other) = delete;

        /**
         * Restore the current thread to its previous priority level
         */
        ~ThreadPriority() noexcept;

    private:
        /// The priority level to restore to
        PriorityLevel previous_priority;

        /// The current priority level as set by an instance of this class
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        static thread_local PriorityLevel current_priority;
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_THREAD_PRIORITY_HPP
