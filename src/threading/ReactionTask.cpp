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

#include "ReactionTask.hpp"

#include <atomic>
#include <functional>
#include <utility>

#include "../id.hpp"
#include "../message/ReactionStatistics.hpp"
#include "../util/platform.hpp"
#include "Reaction.hpp"

namespace NUClear {
namespace threading {

    const ReactionTask* ReactionTask::get_current_task() {
        return current_task;
    }

    ReactionTask::~ReactionTask() {
        // Decrement the number of active tasks
        if (parent != nullptr) {
            parent->active_tasks.fetch_sub(1, std::memory_order_release);
        }
    }

    void ReactionTask::run() noexcept {
        // Update the current task
        auto* t = std::exchange(current_task, this);
        try {
            // Run our callback
            callback(*this);
        }
        catch (...) {  // NOLINT(bugprone-empty-catch)
            // This shouldn't happen, but either way no exceptions should ever leave this function
            // They should have all been caught and callback is noexcept
            // However somehow it still happens sometimes so we need to catch it
        }

        // Restore the current task
        current_task = t;
    }

    NUClear::id_t ReactionTask::next_id() {
        // Start at 1 to make 0 an invalid id
        static std::atomic<NUClear::id_t> id_source(1);
        return id_source.fetch_add(1, std::memory_order_seq_cst);
    }

    std::shared_ptr<message::ReactionStatistics> ReactionTask::make_statistics() {

        // Stats are disabled if they are disabled in the parent or in the causing task
        if ((parent != nullptr && !parent->emit_stats)
            || (current_task != nullptr && current_task->statistics == nullptr)) {
            return nullptr;
        }

        // Identifiers come from the parent reaction if there is one
        auto identifiers = parent != nullptr ? parent->identifiers : nullptr;

        const auto* ct                = current_task;
        const auto cause_reaction_id  = ct != nullptr && ct->parent != nullptr ? ct->parent->id : 0;
        const auto cause_task_id      = current_task != nullptr ? current_task->id : 0;
        const auto target_reaction_id = parent != nullptr ? parent->id : 0;
        const auto target_task_id     = id;

        return std::make_shared<message::ReactionStatistics>(identifiers,
                                                             IDPair{cause_reaction_id, cause_task_id},
                                                             IDPair{target_reaction_id, target_task_id},
                                                             pool_descriptor,
                                                             group_descriptors);
    }

    // Initialize our current task
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
    ATTRIBUTE_TLS ReactionTask* ReactionTask::current_task = nullptr;

}  // namespace threading
}  // namespace NUClear
