/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
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

#include "Reaction.hpp"

#include <memory>
#include <utility>

#include "../id.hpp"
#include "ReactionIdentifiers.hpp"
#include "ReactionTask.hpp"

namespace NUClear {
namespace threading {

    Reaction::Reaction(Reactor& reactor, ReactionIdentifiers&& identifiers, TaskGenerator&& generator)
        : reactor(reactor)
        , identifiers(std::make_shared<ReactionIdentifiers>(std::move(identifiers)))
        , generator(std::move(generator)) {}

    Reaction::~Reaction() = default;

    std::unique_ptr<ReactionTask> Reaction::get_task(const bool& request_inline) {
        // If we are not enabled, don't run
        if (!enabled) {
            return nullptr;
        }

        // Return the task returned by the generator
        return generator(this->shared_from_this(), request_inline);
    }

    void Reaction::unbind() {
        // Unbind
        for (auto& u : unbinders) {
            u(*this);
        }
    }

    bool Reaction::is_enabled() const {
        return enabled;
    }

    NUClear::id_t Reaction::next_id() {
        // Start at 1 to make 0 an invalid id
        static std::atomic<NUClear::id_t> id_source(1);
        return id_source.fetch_add(1, std::memory_order_seq_cst);
    }

}  // namespace threading
}  // namespace NUClear
