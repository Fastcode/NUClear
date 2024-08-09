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

#ifndef NUCLEAR_THREADING_REACTION_TASK_HPP
#define NUCLEAR_THREADING_REACTION_TASK_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <set>

#include "../id.hpp"
#include "../util/GroupDescriptor.hpp"
#include "../util/ThreadPoolDescriptor.hpp"
#include "../util/platform.hpp"
#include "Reaction.hpp"

namespace NUClear {
namespace message {
    struct ReactionStatistics;
}  // namespace message

namespace threading {

    /**
     * This is a data-bound call of a Reaction ready to be executed.
     *
     * This class holds a reaction that is ready to be executed. It is a Reaction object which has had it's callback
     * parameters bound with data. This can then be executed as a function to run the call inside it.
     */
    class ReactionTask {
    private:
        /// the current task that is being executed by this thread (or nullptr if none is)
        static ATTRIBUTE_TLS ReactionTask* current_task;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    public:
        /// Type of the functions that ReactionTasks execute
        using TaskFunction = std::function<void(ReactionTask&) noexcept>;

        /**
         * Gets the current executing task, or nullptr if there isn't one.
         *
         * @return the current executing task or nullptr if there isn't one
         */
        static const ReactionTask* get_current_task();

        /**
         * Creates a new ReactionTask object bound with the parent Reaction object (that created it) and task.
         *
         * @param parent            the Reaction object that spawned this ReactionTask.
         * @param priority          a function that can be called to get the priority of this task
         * @param pool_descriptor   a function that can be called to get the thread pool descriptor for this task
         * @param group_descriptors a function that can be called to get the list of group descriptors for this task
         */
        template <typename GetPriority, typename GetGroups, typename GetThreadPool>
        ReactionTask(const std::shared_ptr<Reaction>& parent,
                     const GetPriority& priority_fn,
                     const GetThreadPool& thread_pool_fn,
                     const GetGroups& groups_fn)
            : parent(parent)
            , id(new_task_id())
            , priority(priority_fn(*this))
            , pool_descriptor(thread_pool_fn(*this))
            , group_descriptors(groups_fn(*this))
            // Only create a stats object if we wouldn't cause an infinite loop of stats
            , stats(
                  parent != nullptr && parent->emit_stats && (current_task == nullptr || current_task->stats != nullptr)
                      ? std::make_shared<message::ReactionStatistics>(
                            parent->identifiers,
                            IDPair{parent->id, id},
                            current_task != nullptr ? IDPair{current_task->parent->id, current_task->id} : IDPair{0, 0},
                            pool_descriptor,
                            group_descriptors)
                      : nullptr) {
            // Increment the number of active tasks
            if (parent != nullptr) {
                parent->active_tasks.fetch_add(1, std::memory_order_release);
            }
        }

        // No copying or moving of tasks (use unique_ptrs to manage tasks)
        ReactionTask(const ReactionTask&)            = delete;
        ReactionTask& operator=(const ReactionTask&) = delete;
        ReactionTask(ReactionTask&&)                 = delete;
        ReactionTask& operator=(ReactionTask&&)      = delete;

        /**
         * Destructor for the ReactionTask object.
         *
         * This will decrement the active_tasks counter on the parent Reaction object.
         */
        ~ReactionTask();

        /**
         * Runs the internal data bound task.
         */
        void run();

        /**
         * Generate a new unique task id
         *
         * @return a new unique task id
         */
        static NUClear::id_t new_task_id();

        /// the parent Reaction object which spawned this, or nullptr if this is a floating task
        std::shared_ptr<Reaction> parent;
        /// the task id of this task (the sequence number of this particular task)
        NUClear::id_t id;

        /// the priority to run this task at
        int priority;
        /// details about the thread pool that this task will run from, this will also influence what task queue
        /// the tasks will be queued on
        util::ThreadPoolDescriptor pool_descriptor;
        /// details about the groups that this task will run in
        std::set<util::GroupDescriptor> group_descriptors;

        /// the statistics object that records run details about this reaction task
        // This will be nullptr if this task is ineligible to emit stats (e.g. it would cause a loop)
        std::shared_ptr<message::ReactionStatistics> stats;

        /// the data bound callback to be executed
        /// @attention note this must be last in the list as the this pointer is passed to the callback generator
        TaskFunction callback;

        /**
         * This operator compares two ReactionTask objects based on their priority and ID.
         *
         * It sorts tasks in the order that they should be executed by comparing their priority the the order they were
         * created. The task that should execute first will have the lowest sort order value and the task that should
         * execute last will have the highest sort order.
         *
         * The task with higher priority is considered less.
         * If two tasks have equal priority, the one with the lower ID is considered less.
         *
         * @param other The other ReactionTask object to compare with.
         *
         * @return true if the current object is less than the other object, false otherwise.
         */
        bool operator<(const ReactionTask& other) const {
            return priority == other.priority ? id < other.id : priority > other.priority;
        }
    };

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_REACTION_TASK_HPP
