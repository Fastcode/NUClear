/*
 * Copyright (C) 2013-2016 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_UTIL_CALLBACKGENERATOR_HPP
#define NUCLEAR_UTIL_CALLBACKGENERATOR_HPP

#include "nuclear_bits/dsl/trait/is_transient.hpp"
#include "nuclear_bits/dsl/word/emit/Direct.hpp"
#include "nuclear_bits/util/MergeTransient.hpp"
#include "nuclear_bits/util/TransientDataElements.hpp"
#include "nuclear_bits/util/apply.hpp"
#include "nuclear_bits/util/demangle.hpp"
#include "nuclear_bits/util/update_current_thread_priority.hpp"

namespace NUClear {
namespace util {

    template <size_t I = 0, typename... T>
    inline typename std::enable_if<I == sizeof...(T), bool>::type check_data(const std::tuple<T...>&) {
        return true;
    }

    template <size_t I = 0, typename... T>
        inline typename std::enable_if < I<sizeof...(T), bool>::type check_data(const std::tuple<T...>& t) {
        return std::get<I>(t) && check_data<I + 1>(t);
    }


    template <typename DSL, typename Function>
    struct CallbackGenerator {

        CallbackGenerator(Function&& callback)
            : callback(std::forward<Function>(callback))
            , transients(std::make_shared<typename TransientDataElements<DSL>::type>()){};

        template <typename... T, int... DIndex, int... Index>
        void merge_transients(std::tuple<T...>& data, const Sequence<DIndex...>&, const Sequence<Index...>&) {

            // Merge our transient data
            unpack(MergeTransients<std::remove_reference_t<decltype(std::get<DIndex>(data))>>::merge(
                std::get<Index>(*transients), std::get<DIndex>(data))...);
        }


        std::pair<int, threading::ReactionTask::TaskFunction> operator()(threading::Reaction& r) {

            // Check if we should even run
            if (!DSL::precondition(r)) {
                // We cancel our execution by returning an empty function
                return std::make_pair(0, threading::ReactionTask::TaskFunction());
            }
            else {

                // Bind our data to a variable (this will run in the dispatching thread)
                auto data = DSL::get(r);

                // Merge our transient data in
                merge_transients(data,
                                 typename TransientDataElements<DSL>::index(),
                                 GenerateSequence<0, TransientDataElements<DSL>::index::length>());

                // Check if our data is good (all the data exists) otherwise terminate the call
                if (!check_data(data)) {
                    // We cancel our execution by returning an empty function
                    return std::make_pair(0, threading::ReactionTask::TaskFunction());
                }

                // We have to make a copy of the callback because the "this" variable can go out of scope
                auto c = callback;
                return std::make_pair(DSL::priority(r), [c, data](std::unique_ptr<threading::ReactionTask>&& task) {

                    // Check if we are going to reschedule
                    task = DSL::reschedule(std::move(task));

                    // If we still control our task
                    if (task) {

                        // Update our thread's priority to the correct level
                        update_current_thread_priority(task->priority);

                        // Record our start time
                        task->stats->started = clock::now();

                        // We have to catch any exceptions
                        try {
                            // We call with only the relevant arguments to the passed function
                            util::apply_relevant(c, std::move(data));
                        }
                        catch (...) {

                            // Catch our exception if it happens
                            task->stats->exception = std::current_exception();
                        }

                        // Our finish time
                        task->stats->finished = clock::now();

                        // Run our postconditions
                        DSL::postcondition(*task);

                        // Emit our reaction statistics
                        PowerPlant::powerplant->emit<dsl::word::emit::Direct>(task->stats);
                    }

                    // Return our task
                    return std::move(task);
                });
            }
        }

        Function callback;
        std::shared_ptr<typename TransientDataElements<DSL>::type> transients;
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_CALLBACKGENERATOR_HPP
