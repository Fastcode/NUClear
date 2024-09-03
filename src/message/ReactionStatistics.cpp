/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#include "ReactionStatistics.hpp"

#include "../threading/scheduler/Pool.hpp"

namespace NUClear {
namespace message {

    ReactionStatistics::Event ReactionStatistics::Event::now() {

        return Event{
            Event::ThreadInfo{
                std::this_thread::get_id(),
                threading::scheduler::Pool::current() != nullptr ? threading::scheduler::Pool::current()->descriptor
                                                                 : nullptr,
            },
            NUClear::clock::now(),
            std::chrono::steady_clock::now(),
            util::cpu_clock::now(),
        };
    }

    ReactionStatistics::ReactionStatistics(std::shared_ptr<const threading::ReactionIdentifiers> identifiers,
                                           const IDPair& cause,
                                           const IDPair& target,
                                           std::shared_ptr<const util::ThreadPoolDescriptor> target_pool,
                                           std::set<std::shared_ptr<const util::GroupDescriptor>> target_groups)
        : identifiers(std::move(identifiers))
        , cause(cause)
        , target(target)
        , target_pool(std::move(target_pool))
        , target_groups(std::move(target_groups))
        , created(Event::now()) {}

}  // namespace message
}  // namespace NUClear
