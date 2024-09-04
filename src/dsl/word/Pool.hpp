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

#ifndef NUCLEAR_DSL_WORD_POOL_HPP
#define NUCLEAR_DSL_WORD_POOL_HPP

#include <map>
#include <mutex>
#include <typeindex>

#include "../../threading/ReactionTask.hpp"
#include "../../util/ThreadPoolDescriptor.hpp"
#include "../../util/demangle.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        namespace pool {
            /**
             * Thread pool descriptor for the default thread pool
             */
            struct Default {
                static constexpr const char* name = "Default";
                static constexpr int concurrency  = 0;
            };
        }  // namespace pool

        /**
         * This is used to specify that this reaction should run in the designated thread pool
         *
         * @code on<Trigger<T, ...>, Pool<PoolType>>() @endcode
         * This DSL will cause the creation of a new thread pool with a specific number of threads allocated to it.
         *
         * All tasks for this reaction will be queued to run on threads from this thread pool.
         *
         * Tasks in the queue are ordered based on their priority level, then their task id.
         *
         * When this DSL is not specified the default thread pool will be used.
         * For tasks that need to run on the main thread use MainThread.
         *
         * @attention
         *  This DSL should be used sparingly as having an increased number of threads running concurrently on the
         *  system can lead to a degradation in performance.
         *
         * @par Implements
         *  pool
         *
         * @tparam PoolType A struct that contains the details of the thread pool to create.
         *                  This struct should contain a static int member that sets the number of threads that
         * should be allocated to this pool.
         *  @code
         *  struct ThreadPool {
         *      static constexpr int concurrency = 2;
         *  };
         *  @endcode
         */
        template <typename PoolType = pool::Default>
        struct Pool {

            // This must be a separate function, otherwise each instance of DSL will be a separate pool
            static std::shared_ptr<const util::ThreadPoolDescriptor> descriptor() {
                static const auto pool_descriptor =
                    std::make_shared<const util::ThreadPoolDescriptor>(name<PoolType>(),
                                                                       concurrency<PoolType>(),
                                                                       counts_for_idle<PoolType>(),
                                                                       persistent<PoolType>());
                return pool_descriptor;
            }

            template <typename DSL>
            static std::shared_ptr<const util::ThreadPoolDescriptor> pool(const threading::ReactionTask& /*task*/) {
                return descriptor();
            }

        private:
            template <typename U>
            static auto name() -> decltype(U::name) {
                return U::name;
            }
            template <typename U, typename... A>
            static std::string name(const A&... /*unused*/) {
                return util::demangle(typeid(U).name());
            }

            template <typename U>
            static constexpr auto concurrency() -> decltype(U::concurrency) {
                return U::concurrency;
            }
            // No default for thread count

            template <typename U>
            static constexpr auto counts_for_idle() -> decltype(U::counts_for_idle) {
                return U::counts_for_idle;
            }
            template <typename U, typename... A>
            static constexpr bool counts_for_idle(const A&... /*unused*/) {
                return true;
            }

            template <typename U>
            static constexpr auto persistent() -> decltype(U::persistent) {
                return U::persistent;
            }
            template <typename U, typename... A>
            static constexpr bool persistent(const A&... /*unused*/) {
                return false;
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_POOL_HPP
