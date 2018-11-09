/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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
#include "Reaction.hpp"

#include <utility>

namespace NUClear {
namespace threading {

    // Initialize our reaction source
    std::atomic<uint64_t> Reaction::reaction_id_source(0);  // NOLINT

    Reaction::Reaction(Reactor& reactor, std::vector<std::string>&& identifier, TaskGenerator&& generator)
        : reactor(reactor)
        , identifier(identifier)
        , id(++reaction_id_source)
        , emit_stats(true)
        , active_tasks(0)
        , enabled(true)
        , generator(generator) {}

    void Reaction::unbind() {
        // Unbind
        for (auto& u : unbinders) {
            u(*this);
        }
    }

    std::unique_ptr<ReactionTask> Reaction::get_task() {

        // If we are not enabled, don't run
        if (!enabled) { return std::unique_ptr<ReactionTask>(nullptr); }

        // Run our generator to get a functor we can run
        int priority;
        std::function<std::unique_ptr<ReactionTask>(std::unique_ptr<ReactionTask> &&)> func;
        std::tie(priority, func) = generator(*this);

        // If our generator returns a valid function
        if (func) { return std::make_unique<ReactionTask>(*this, priority, std::move(func)); }

        // Otherwise we return a null pointer
        return std::unique_ptr<ReactionTask>(nullptr);
    }

    bool Reaction::is_enabled() {
        return enabled;
    }
}  // namespace threading
}  // namespace NUClear
