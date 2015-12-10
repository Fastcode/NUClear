/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#include "nuclear_bits/extension/ChronoController.hpp"

#include <algorithm>

#include "nuclear_bits/dsl/word/Every.hpp"

namespace NUClear {
    namespace extension {

        ChronoController::ChronoController(std::unique_ptr<NUClear::Environment> environment)
            : Reactor(std::move(environment))
            , steps(0) {

            on<Trigger<dsl::word::EveryConfiguration>>().then("Configure Every Reaction", [this] (const dsl::word::EveryConfiguration& config) {

                auto item = std::find_if(std::begin(steps), std::end(steps), [&config] (const Step& item) {
                    return item.jump == config.jump;
                });

                // If we haven't got this duration step yet
                if(item == std::end(steps) ) {
                    steps.push_back(Step {
                        config.jump,
                        clock::now(),
                        std::vector<std::shared_ptr<threading::Reaction>>()
                    });
                    steps.back().reactions.push_back(std::move(config.reaction));
                }
                else {
                    item->reactions.push_back(std::move(config.reaction));
                }

                // Sort the steps
                std::sort(std::begin(steps), std::end(steps));

                // Poke the system
                wait.notify_all();
            });

            on<Trigger<dsl::operation::Unbind<Every<>>>>().then("Unbind Every Reaction", [this] (const dsl::operation::Unbind<Every<>>& unbind) {

                // Loop through all of our steps
                for(auto& step : steps) {

                    // See if this step has the target reaction
                    auto item = std::find_if(std::begin(step.reactions), std::end(step.reactions), [unbind] (const std::shared_ptr<threading::Reaction>& r) {
                        return r->reactionId == unbind.reactionId;
                    });

                    // If we have this item then remove it
                    if(item != std::end(step.reactions)) {
                        step.reactions.erase(item);
                    }
                }
            });

            // When we shutdown we notify so we quit now
            on<Shutdown>().then("Shutdown Chrono Controller", [this] {
                wait.notify_all();
            });

            on<Always>().then("Chrono Controller", [this] {

                // Aquire the mutex lock so we can wait on it
                std::unique_lock<std::mutex> lock(mutex);

                // If we have steps to do
                if(!steps.empty()) {

                    // Wait until the next event
                    wait.wait_for(lock, steps.front().next - clock::now());

                    // Get the current time
                    clock::time_point now(clock::now());

                    // Busy wait for the time to be right to improve accuracy
                    while (now < steps.front().next) {
                        now = clock::now();
                    };

                    // Check if any intervals are before now and if so execute their callbacks and add their step.
                    for(auto& step : steps) {
                        if((step.next - now).count() <= 0) {
                            for(auto& reaction : step.reactions) {

                                try {
                                    // submit the reaction to the thread pool
                                    auto task = reaction->getTask();
                                    if(task) {
                                        powerplant.submit(std::move(task));
                                    }
                                }
                                catch(...) {
                                }
                            }
                            step.next += step.jump;
                        }
                        // Since we are sorted, we can ignore any after this time
                        else {
                            break;
                        }
                    }

                    // Sort the steps
                    std::sort(std::begin(steps), std::end(steps));
                }
                // Otherwise we wait for something to happen
                else {
                    wait.wait(lock);
                }
            });

        }
    }
}
