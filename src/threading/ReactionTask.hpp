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
    class TReactionTask {
    private:
        /// @brief a source for task ids, atomically creates longs
        static std::atomic<std::uint64_t> task_id_source;

        /// @brief the current task that is being executed by this thread (or nullptr if none is)
        static ATTRIBUTE_TLS TReactionTask* current_task;

    public:
        /// Type of the functions that ReactionTasks execute
        using TaskFunction =
            std::function<std::unique_ptr<TReactionTask<ReactionType>>(std::unique_ptr<TReactionTask<ReactionType>>&&)>;

        /**
         * @brief Gets the current executing task, or nullptr if there isn't one.
         *
         * @return the current executing task or nullptr if there isn't one
         */
        static const TReactionTask* get_current_task() {
            return current_task;
        }

        /**
         * @brief Creates a new ReactionTask object bound with the parent Reaction object (that created it) and task.
         *
         * @param parent    the Reaction object that spawned this ReactionTask.
         * @param priority  the priority to use when executing this task.
         * @param callback  the data bound callback to be executed in the threadpool.
         */
        TReactionTask(ReactionType& parent, int priority, TaskFunction&& callback)
            : parent(parent)
            , id(++task_id_source)
            , priority(priority)
            , stats(new message::ReactionStatistics{parent.identifier,
                                                    parent.id,
                                                    id,
                                                    current_task != nullptr ? current_task->parent.id : 0,
                                                    current_task != nullptr ? current_task->id : 0,
                                                    clock::now(),
                                                    clock::time_point(std::chrono::seconds(0)),
                                                    clock::time_point(std::chrono::seconds(0)),
                                                    nullptr})
            , emit_stats(parent.emit_stats && (current_task != nullptr ? current_task->emit_stats : true))
            , callback(callback) {}


        /**
         * @brief Runs the internal data bound task and times it.
         *
         * @details
         *  This runs the internal data bound task and times how long the execution takes. These figures can then be
         *  used in a debugging context to calculate how long callbacks are taking to run.
         */
        inline std::unique_ptr<TReactionTask<ReactionType>> run(std::unique_ptr<TReactionTask<ReactionType>>&& us) {

            // Update our current task
            TReactionTask* old_task = current_task;
            current_task            = this;

            // Run our callback at catch the returned task (to see if it rescheduled itself)
            us = callback(std::move(us));

            // Reset our task back
            current_task = old_task;

            // Return our original task
            return std::move(us);
        }

        /// @brief the parent Reaction object which spawned this
        ReactionType& parent;
        /// @brief the task id of this task (the sequence number of this particular task)
        uint64_t id;
        /// @brief the priority to run this task at
        int priority;
        /// @brief the statistics object that persists after this for information and debugging
        std::unique_ptr<message::ReactionStatistics> stats;
        /// @brief if these stats are safe to emit. It should start true, and as soon as we are a reaction based on
        /// reaction statistics becomes false for all created tasks. This is to stop infinite loops of death.
        bool emit_stats;

        /// @brief the data bound callback to be executed
        /// @attention note this must be last in the list as the this pointer is passed to the callback generator
        TaskFunction callback;
    };

    // Initialize our id source
    template <typename ReactionType>
    std::atomic<uint64_t> TReactionTask<ReactionType>::task_id_source(0);  // NOLINT

    // Initialize our current task
    template <typename ReactionType>
    ATTRIBUTE_TLS TReactionTask<ReactionType>* TReactionTask<ReactionType>::current_task = nullptr;  // NOLINT

    /**
     * @brief This overload is used to sort reactions by priority.
     *
     * @param a the reaction task a
     * @param b the reaction task b
     *
     * @return true if a has lower priority than b, false otherwise
     */
    template <typename ReactionType>
    inline bool operator<(const std::unique_ptr<TReactionTask<ReactionType>>& a,
                          const std::unique_ptr<TReactionTask<ReactionType>>& b) {

        // If we ever have a null pointer, we move it to the top of the queue as it is being removed
        return a == nullptr                 ? false
               : b == nullptr               ? true
               : a->priority == b->priority ? a->stats->emitted > b->stats->emitted
                                            : a->priority < b->priority;
    }
}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_REACTIONTASK_HPP
