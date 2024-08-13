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
     * This is a databound call of a Reaction ready to be executed.
     *
     * This class holds a reaction that is ready to be executed.
     * It is a Reaction object which has had it's callback parameters bound with data.
     * This can then be executed as a function to run the call inside it.
     */
    class ReactionTask {
    private:
        /// The current task that is being executed by this thread (or nullptr if none is)
        static ATTRIBUTE_TLS ReactionTask* current_task;  // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

    public:
        /// Type of the functions that ReactionTasks execute
        using TaskFunction = std::function<void(ReactionTask&)>;

        /**
         * Gets the current executing task, or nullptr if there isn't one.
         *
         * @return The current executing task or nullptr if there isn't one
         */
        static const ReactionTask* get_current_task();

        /**
         * Creates a new ReactionTask object bound with the parent Reaction object (that created it) and task.
         *
         * @param parent                 The Reaction object that spawned this ReactionTask
         * @param priority               The priority to use when executing this task
         * @param group_descriptor       The descriptor for the group that this task should run in
         * @param thread_pool_descriptor The descriptor for the thread pool that this task should be queued in
         * @param callback               The data bound callback to be executed in the thread pool
         */
        ReactionTask(Reaction& parent,
                     const int& priority,
                     const util::GroupDescriptor& group_descriptor,
                     const util::ThreadPoolDescriptor& thread_pool_descriptor,
                     TaskFunction&& callback);


        /**
         * Runs the internal data bound task and times it.
         *
         * This runs the internal data bound task and times how long the execution takes.
         * These figures can then be used in a debugging context to calculate how long callbacks are taking to run.
         */
        void run();

        /**
         * Generate a new unique task id.
         *
         * @return A new unique task id
         */
        static NUClear::id_t new_task_id();

        /// The parent Reaction object which spawned this
        Reaction& parent;
        /// The task id of this task (the sequence number of this particular task)
        NUClear::id_t id{new_task_id()};
        /// The priority to run this task at
        int priority;
        /// The statistics object that persists after this for information and debugging
        std::shared_ptr<message::ReactionStatistics> stats;
        /// If these stats are safe to emit. It should start true, and as soon as we are a reaction based on
        /// reaction statistics becomes false for all created tasks.
        /// This is to stop infinite loops tasks triggering tasks.
        bool emit_stats;

        /// Details about the group that this task will run in
        util::GroupDescriptor group_descriptor;

        /// Details about the thread pool that this task will run from, this will also influence what task queue the
        /// tasks will be queued on
        util::ThreadPoolDescriptor thread_pool_descriptor;

        /// The data bound callback to be executed
        /// @attention note this must be last in the list as the this pointer is passed to the callback generator
        TaskFunction callback;
    };

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_REACTION_TASK_HPP
