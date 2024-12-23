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

#ifndef NUCLEAR_EXTENSION_CHRONO_CONTROLLER_HPP
#define NUCLEAR_EXTENSION_CHRONO_CONTROLLER_HPP

#include <memory>

#include "../Reactor.hpp"
#include "../message/TimeTravel.hpp"
#include "../util/Sleeper.hpp"

namespace NUClear {
namespace extension {

    class ChronoController : public Reactor {
    private:
        using ChronoTask = NUClear::dsl::operation::ChronoTask;
        using Unbind     = NUClear::dsl::operation::Unbind<ChronoTask>;

    public:
        explicit ChronoController(std::unique_ptr<NUClear::Environment> environment);

    private:
        /**
         * Add a new task to the ChronoController.
         *
         * @param task The task to add
         */
        void add(const std::shared_ptr<ChronoTask>& task);

        /**
         * Remove a task from the ChronoController.
         *
         * @param id The id of the task to remove
         */
        void remove(const NUClear::id_t& id);

        /**
         * Try to run the next task in the list
         *
         * @return the time point of the next task to run or max if there are no tasks
         */
        NUClear::clock::time_point next();

        /// The mutex used to guard the tasks and running flag
        std::mutex mutex;
        /// The list of tasks we need to process
        std::vector<std::shared_ptr<dsl::operation::ChronoTask>> tasks;

        /// If we are running or not
        std::atomic<bool> running{true};

        /// The class which is able to perform high precision sleeps
        util::Sleeper sleeper;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_CHRONO_CONTROLLER_HPP
