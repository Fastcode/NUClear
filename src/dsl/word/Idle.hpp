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

#ifndef NUCLEAR_DSL_WORD_IDLE_HPP
#define NUCLEAR_DSL_WORD_IDLE_HPP

#include <map>
#include <mutex>
#include <typeindex>

#include "../../threading/ReactionTask.hpp"
#include "../fusion/NoOp.hpp"
#include "MainThread.hpp"
#include "Pool.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * A base type to handle the common code for idling after turning the pool descriptor into an id.
         *
         * @param reaction        The reaction to bind the idle task to
         * @param pool_descriptor The descriptor that was used to create the thread pool.
         */
        inline void bind_idle(const std::shared_ptr<threading::Reaction>& reaction,
                              const std::shared_ptr<const util::ThreadPoolDescriptor>& pool_descriptor) {

            // Our unbinder to remove this reaction
            reaction->unbinders.push_back([pool_descriptor](const threading::Reaction& r) {  //
                r.reactor.powerplant.remove_idle_task(r.id, pool_descriptor);
            });

            reaction->reactor.powerplant.add_idle_task(reaction, pool_descriptor);
        }

        /**
         * Execute a task when there is nothing currently running on the thread pool.
         *
         * @code on<Idle<PoolType>>() @endcode
         * When the thread pool is idle, this task will be executed.
         *
         * @par Implements
         *  Bind
         *
         * @tparam PoolType The descriptor that was used to create the thread pool.
         *                  `void` for the default pool MainThread for the main thread pool.
         */
        template <typename PoolType>
        struct Idle {
            template <typename DSL>
            static void bind(const std::shared_ptr<threading::Reaction>& reaction) {

                // Make a fake task to use for finding an appropriate descriptor
                threading::ReactionTask task(reaction, false, DSL::priority, DSL::run_inline, DSL::pool, DSL::group);
                bind_idle(reaction, PoolType::template pool<DSL>(task));
            }
        };

        template <>
        struct Idle<void> {
            template <typename DSL>
            static void bind(const std::shared_ptr<threading::Reaction>& reaction) {
                bind_idle(reaction, nullptr);
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_IDLE_HPP
