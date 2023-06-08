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

#ifndef NUCLEAR_UTIL_GENERATEDCALLBACK_HPP
#define NUCLEAR_UTIL_GENERATEDCALLBACK_HPP

#include <typeindex>

#include "../threading/ReactionTask.hpp"
#include "thread_pool.hpp"

namespace NUClear {
namespace util {

    struct GeneratedCallback {
        GeneratedCallback() = default;
        GeneratedCallback(const int& priority,
                          const std::type_index& group,
                          const ThreadPoolDescriptor& pool,
                          threading::ReactionTask::TaskFunction callback)
            : priority(priority), group(group), pool(pool), callback(std::move(callback)) {}
        int priority{0};
        std::type_index group{typeid(GeneratedCallback)};
        ThreadPoolDescriptor pool{util::ThreadPoolIDSource::DEFAULT_THREAD_POOL_ID, 0};
        threading::ReactionTask::TaskFunction callback{};

        operator bool() const {
            return bool(callback);
        }
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_GENERATEDCALLBACK_HPP
