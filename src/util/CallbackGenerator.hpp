/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_CALLBACK_GENERATOR_HPP
#define NUCLEAR_UTIL_CALLBACK_GENERATOR_HPP

#include <type_traits>

#include "../dsl/word/emit/Direct.hpp"
#include "../message/ReactionStatistics.hpp"
#include "../util/MergeTransient.hpp"
#include "../util/TransientDataElements.hpp"
#include "../util/apply.hpp"
#include "../util/unpack.hpp"
#include "../util/update_current_thread_priority.hpp"

namespace NUClear {
namespace util {

    template <size_t I = 0, typename... T>
    inline std::enable_if_t<(I == sizeof...(T)), bool> check_data(const std::tuple<T...>& /*t*/) {
        return true;
    }

    template <size_t I = 0, typename... T>
    inline std::enable_if_t<(I < sizeof...(T)), bool> check_data(const std::tuple<T...>& t) {
        return std::get<I>(t) && check_data<I + 1>(t);
    }


    template <typename DSL, typename Function>
    struct CallbackGenerator {

        // Don't use this constructor if F is of type CallbackGenerator
        template <
            typename F,
            std::enable_if_t<!std::is_same<std::remove_reference_t<std::remove_cv_t<F>>, CallbackGenerator>::value,
                             bool> = true>
        explicit CallbackGenerator(F&& callback) : callback(std::forward<F>(callback)) {}

        template <typename... T, int... DIndex, int... Index>
        void merge_transients(std::tuple<T...>& data,
                              const Sequence<DIndex...>& /*d*/,
                              const Sequence<Index...>& /*i*/) {

            // Merge our transient data
            unpack(MergeTransients<std::remove_reference_t<decltype(std::get<DIndex>(data))>>::merge(
                std::get<Index>(*transients),
                std::get<DIndex>(data))...);
        }

        std::unique_ptr<threading::ReactionTask> operator()(const std::shared_ptr<threading::Reaction>& r) {

            auto task = std::make_unique<threading::ReactionTask>(r, DSL::priority, DSL::pool, DSL::group);

            // Check if we should even run
            if (!DSL::precondition(*task)) {

                // Set the created status as rejected and emit it
                if (task->stats != nullptr) {
                    PowerPlant::powerplant->emit<dsl::word::emit::Direct>(
                        std::make_unique<message::ReactionEvent>(message::ReactionEvent::BLOCKED, task->stats));
                }

                // Nothing to run
                return nullptr;
            }

            // Bind our data to a variable (this will run in the dispatching thread)
            auto data = DSL::get(*task);

            // Merge our transient data in
            merge_transients(data,
                             typename TransientDataElements<DSL>::index(),
                             GenerateSequence<0, TransientDataElements<DSL>::index::length>());

            // Check if our data is good (all the data exists) otherwise terminate the call
            if (!check_data(data)) {

                // Set the created status as no data and emit it
                if (task->stats != nullptr) {
                    PowerPlant::powerplant->emit<dsl::word::emit::Direct>(
                        std::make_unique<message::ReactionEvent>(message::ReactionEvent::MISSING_DATA, task->stats));
                }

                // Nothing to run
                return nullptr;
            }

            // Set the created status as no data and emit it
            if (task->stats != nullptr) {
                PowerPlant::powerplant->emit<dsl::word::emit::Direct>(
                    std::make_unique<message::ReactionEvent>(message::ReactionEvent::CREATED, task->stats));
            }

            // We have to make a copy of the callback because the "this" variable can go out of scope
            auto c         = callback;
            task->callback = [c, data](threading::ReactionTask& task) noexcept {
                // Update our thread's priority to the correct level
                update_current_thread_priority(task.priority);

                if (task.stats != nullptr) {
                    task.stats->started = message::ReactionStatistics::Event::now();
                    PowerPlant::powerplant->emit<dsl::word::emit::Direct>(
                        std::make_unique<message::ReactionEvent>(message::ReactionEvent::STARTED, task.stats));
                }

                // We have to catch any exceptions
                try {
                    // We call with only the relevant arguments to the passed function
                    util::apply_relevant(c, std::move(data));
                }
                catch (...) {
                    // Catch our exception if it happens
                    if (task.stats != nullptr) {
                        task.stats->exception = std::current_exception();
                    }
                }

                // Run our postconditions
                DSL::postcondition(task);

                // Emit our reaction statistics if it wouldn't cause a loop
                if (task.stats != nullptr) {
                    task.stats->finished = message::ReactionStatistics::Event::now();
                    PowerPlant::powerplant->emit<dsl::word::emit::Direct>(
                        std::make_unique<message::ReactionEvent>(message::ReactionEvent::FINISHED, task.stats));
                }
            };

            return task;
        }

        Function callback;
        std::shared_ptr<typename TransientDataElements<DSL>::type> transients{
            std::make_shared<typename TransientDataElements<DSL>::type>()};
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_CALLBACK_GENERATOR_HPP
