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

#ifndef NUCLEAR_DSL_WORD_POOL_HPP
#define NUCLEAR_DSL_WORD_POOL_HPP

#include <map>
#include <mutex>
#include <typeindex>

#include "../../threading/ReactionTask.hpp"
#include "../../util/ThreadPoolDescriptor.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        template <typename PoolType>
        struct Pool {
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
            static std::map<std::type_index, uint64_t> pool_id;
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
            static std::mutex mutex;

            template <typename DSL>
            static inline util::ThreadPoolDescriptor pool(threading::Reaction& /*reaction*/) {

                const std::lock_guard<std::mutex> lock(mutex);
                if (pool_id.count(typeid(PoolType)) == 0) {
                    pool_id[typeid(PoolType)] = util::ThreadPoolIDSource::get_new_pool_id();
                }
                return util::ThreadPoolDescriptor{pool_id[typeid(PoolType)], PoolType::concurrency};
            }
        };

        // Initialise the static map and mutex
        template <typename PoolType>
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        std::map<std::type_index, uint64_t> Pool<PoolType>::pool_id;
        template <typename PoolType>
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        std::mutex Pool<PoolType>::mutex;

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_POOL_HPP
