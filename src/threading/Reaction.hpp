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

#ifndef NUCLEAR_THREADING_REACTION_HPP
#define NUCLEAR_THREADING_REACTION_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <vector>

#include "../id.hpp"

namespace NUClear {

// Forward declare reactor
class Reactor;

namespace threading {

    // Forward declare
    class ReactionTask;
    struct ReactionIdentifiers;

    /**
     * This class holds the definition of a Reaction (call signature).
     *
     * A reaction holds the information about a callback. It holds the options as to how to process it in the scheduler.
     * It also holds a function which is used to generate data-bound Task objects.
     * I.e. callback with the function arguments already loaded and ready to run.
     */
    class Reaction : public std::enable_shared_from_this<Reaction> {
        // Reaction handles are given to user code to enable and disable the reaction
        friend class ReactionHandle;
        friend class ReactionTask;

    public:
        // The type of the generator that is used to create functions for ReactionTask objects
        using TaskGenerator = std::function<std::unique_ptr<ReactionTask>(const std::shared_ptr<Reaction>&)>;

        /**
         * Constructs a new Reaction with the passed callback generator and options
         *
         * @param reactor        the reactor this belongs to
         * @param identifiers    string identification information for the reaction
         * @param callback       the callback generator function (creates data-bound callbacks)
         */
        Reaction(Reactor& reactor, ReactionIdentifiers&& identifiers, TaskGenerator&& generator);

        // No copying or moving this class
        Reaction(const Reaction&)            = delete;
        Reaction(Reaction&&)                 = delete;
        Reaction& operator=(const Reaction&) = delete;
        Reaction& operator=(Reaction&&)      = delete;

        // This must go in the c++ file as ReactionTask is an incomplete type at this point
        ~Reaction();

        /**
         * creates a new data-bound callback task that can be executed.
         *
         * @return a unique_ptr to a Task which has the data for it's call bound into it
         */
        std::unique_ptr<ReactionTask> get_task();

        /**
         * returns true if this reaction is currently enabled
         */
        bool is_enabled() const;

        /**
         * Unbinds this reaction from it's context
         */
        void unbind();

        /// the reactor this belongs to
        Reactor& reactor;

        /// This holds the identifying strings for this reaction
        std::shared_ptr<ReactionIdentifiers> identifiers;

        /// the unique identifier for this Reaction object
        const NUClear::id_t id{++reaction_id_source};

        /// if this is false, we cannot emit ReactionStatistics from any reaction triggered by this one
        bool emit_stats{true};

        /// the number of currently active tasks (existing reaction tasks)
        std::atomic<int> active_tasks{0};

        /// if this reaction object is currently enabled
        std::atomic<bool> enabled{true};

        /// list of functions to use to unbind the reaction and clean
        std::vector<std::function<void(Reaction&)>> unbinders;

    private:
        /// a source for reaction_ids, atomically creates ids
        // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        static std::atomic<NUClear::id_t> reaction_id_source;
        /// the callback generator function (creates data-bound callbacks)
        TaskGenerator generator;
    };

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_REACTION_HPP
