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

#ifndef NUCLEAR_THREADING_REACTIONTASK_HPP
#define NUCLEAR_THREADING_REACTIONTASK_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <typeindex>
#include <vector>

#include "../message/ReactionStatistics.hpp"
#include "../util/GroupDescriptor.hpp"
#include "../util/ThreadPoolDescriptor.hpp"
#include "../util/platform.hpp"

namespace NUClear {
namespace threading {

    /**
     * @brief This is a databound call of a Reaction ready to be executed.
     *
     * @tparam ReactionType the type of the reaction
     *
     * @details
     *  This class holds a reaction that is ready to be executed. It is a Reaction object which has had it's callback
     *  parameters bound with data. This can then be executed as a function to run the call inside it.
     */
    template <typename ReactionType>
    class Task {
    private:
        /// @brief a source for task ids, atomically creates longs
        static std::atomic<std::uint64_t> task_id_source;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        /// @brief the current task that is being executed by this thread (or nullptr if none is)
        static ATTRIBUTE_TLS Task* current_task;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    public:
        /// Type of the functions that ReactionTasks execute
        using TaskFunction = std::function<void(Task<ReactionType>&)>;

        /**
         * @brief Gets the current executing task, or nullptr if there isn't one.
         *
         * @return the current executing task or nullptr if there isn't one
         */
        static const Task* get_current_task() {
            return current_task;
        }

        /**
         * @brief Creates a new ReactionTask object bound with the parent Reaction object (that created it) and task.
         *
         * @param parent                 the Reaction object that spawned this ReactionTask.
         * @param priority               the priority to use when executing this task.
         * @param group_descriptor       the descriptor for the group that this task should run in
         * @param thread_pool_descriptor the descriptor for the thread pool that this task should be queued in
         * @param callback               the data bound callback to be executed in the thread pool.
         */
        Task(ReactionType& parent,
             const int& priority,
             const util::GroupDescriptor& group_descriptor,
             const util::ThreadPoolDescriptor& thread_pool_descriptor,
             TaskFunction&& callback)
            : parent(parent)
            , id(++task_id_source)
            , priority(priority)
            , stats(std::make_shared<message::ReactionStatistics>(parent.identifier,
                                                                  parent.id,
                                                                  id,
                                                                  current_task != nullptr ? current_task->parent.id : 0,
                                                                  current_task != nullptr ? current_task->id : 0,
                                                                  clock::now(),
                                                                  clock::time_point(std::chrono::seconds(0)),
                                                                  clock::time_point(std::chrono::seconds(0)),
                                                                  nullptr))
            , emit_stats(parent.emit_stats && (current_task != nullptr ? current_task->emit_stats : true))
            , group_descriptor(group_descriptor)
            , thread_pool_descriptor(thread_pool_descriptor)
            , callback(callback) {}


        /**
         * @brief Runs the internal data bound task and times it.
         *
         * @details
         *  This runs the internal data bound task and times how long the execution takes. These figures can then be
         *  used in a debugging context to calculate how long callbacks are taking to run.
         */
        inline void run() {

            // Update our current task
            const std::shared_ptr<Task> lock(current_task, [](Task* t) { current_task = t; });
            current_task = this;

            // Run our callback
            callback(*this);
        }

        /// @brief the parent Reaction object which spawned this
        ReactionType& parent;
        /// @brief the task id of this task (the sequence number of this particular task)
        uint64_t id;
        /// @brief the priority to run this task at
        int priority;
        /// @brief the statistics object that persists after this for information and debugging
        std::shared_ptr<message::ReactionStatistics> stats;
        /// @brief if these stats are safe to emit. It should start true, and as soon as we are a reaction based on
        /// reaction statistics becomes false for all created tasks. This is to stop infinite loops of death.
        bool emit_stats;

        /// @brief details about the group that this task will run in
        util::GroupDescriptor group_descriptor;

        /// @brief details about the thread pool that this task will run from, this will also influence what task queue
        /// the tasks will be queued on
        util::ThreadPoolDescriptor thread_pool_descriptor;

        /// @brief if this task should run immediately in the current thread. If immediate execution of this task is not
        /// possible (e.g. due to group concurrency restrictions) this task will be queued as normal. This flag will be
        /// set by `emit<Direct>`
        bool immediate{false};

        /// @brief the data bound callback to be executed
        /// @attention note this must be last in the list as the this pointer is passed to the callback generator
        TaskFunction callback;
    };

    // Initialize our id source
    template <typename ReactionType>
    std::atomic<uint64_t> Task<ReactionType>::task_id_source(0);  // NOLINT

    // Initialize our current task
    template <typename ReactionType>
    ATTRIBUTE_TLS Task<ReactionType>* Task<ReactionType>::current_task = nullptr;  // NOLINT

    /**
     * @brief This overload is used to sort reactions by priority.
     *
     * @param a the reaction task a
     * @param b the reaction task b
     *
     * @return true if a has lower priority than b, false otherwise
     */
    template <typename ReactionType>
    inline bool operator<(const std::unique_ptr<Task<ReactionType>>& a, const std::unique_ptr<Task<ReactionType>>& b) {

        // Comparision order is as follows:
        //  nullptr is greater than anything else so it's removed from a queue first
        //  higher priority tasks are greater than lower priority tasks
        //  tasks created first (smaller ids) should run before tasks created later
        return a == nullptr                 ? true
               : b == nullptr               ? false
               : a->priority == b->priority ? a->id < b->id
                                            : a->priority > b->priority;
    }

    // Alias the templated Task so that public API remains intact
    class Reaction;
    using ReactionTask = Task<Reaction>;

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_REACTIONTASK_HPP
