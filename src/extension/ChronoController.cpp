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

#include "nuclear_bits/extension/ChronoController.hpp"

#include <algorithm>

#include "nuclear_bits/dsl/word/Every.hpp"

namespace NUClear {
    namespace extension {

        using NUClear::dsl::word::emit::DelayEmit;

        ChronoController::ChronoController(std::unique_ptr<NUClear::Environment> environment)
        : Reactor(std::move(environment))
        , steps(0)
        , delayEmits(0)
        , mutex()
        , wait()
        , waitOffset(std::chrono::milliseconds(2)) {

            on<Trigger<dsl::word::EveryConfiguration>>().then("Configure Every Reaction", [this] (const dsl::word::EveryConfiguration& config) {

                // Lock the mutex while we're doing stuff
                {
                    std::lock_guard<std::mutex> lock(mutex);

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
                }

                // Poke the system
                wait.notify_all();
            });

            on<Trigger<dsl::operation::Unbind<Every<>>>>().then("Unbind Every Reaction", [this] (const dsl::operation::Unbind<Every<>>& unbind) {

                // Lock the mutex while we're doing stuff
                {
                    std::lock_guard<std::mutex> lock(mutex);

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
                }

                // Poke the system to make sure it's not waiting on something that's gone
                wait.notify_all();
            });

            on<Trigger<DelayEmit>>().then("Add Delay Emit", [this] (const std::shared_ptr<const DelayEmit>& delayEmit) {

                // Lock the mutex while we're doing stuff
                {
                    std::lock_guard<std::mutex> lock(mutex);

                    // add the delay emit to the list of things to do
                    delayEmits.push_back(delayEmit);

                    // Sort by time so the soonest one is first
                    std::sort(delayEmits.begin(), delayEmits.end(), [] (const std::shared_ptr<const DelayEmit>& a, const std::shared_ptr<const DelayEmit>& b) {
                        return a->time < b->time;
                    });
                }

                // poke the system so it will wait on the correct time
                wait.notify_all();
            });

            // When we shutdown we notify so we quit now
            on<Shutdown>().then("Shutdown Chrono Controller", [this] {
                wait.notify_all();
            });

            on<Always, Priority::REALTIME>().then("Chrono Controller", [this] {

                // Aquire the mutex lock so we can wait on it
                std::unique_lock<std::mutex> lock(mutex);

                // If we have steps to do or delay emits to do look at them otherwise wait
                if(!steps.empty() || !delayEmits.empty()) {

                    // TODO work out which is soonest out of delay emit and every steps and wait for that one
                    NUClear::clock::time_point next = steps.empty() ? delayEmits.front()->time : steps.front().next;

                    // If we are within the wait offset of the time, spinlock until we get there for greater accuracy
                    if (NUClear::clock::now() - next < waitOffset) {
                        
                        // Spinlock!
                        while (NUClear::clock::now() < next);

                        NUClear::clock::time_point now = NUClear::clock::now();

                        // Check if any every intervals are before now and if so execute their callbacks and add their step.
                        for (auto& step : steps) {
                            if ((step.next - now).count() <= 0) {

                                for (auto& reaction : step.reactions) {
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

                        // Sort the steps for next time
                        std::sort(std::begin(steps), std::end(steps));

                        // Now work out if we have any delayed emits to do
                        for (auto it = delayEmits.begin(); it != delayEmits.end();) {
                            if (((*it)->time - now).count() <= 0) {
                                (*it)->emit();
                                it = delayEmits.erase(it);
                            }
                            // The list is sorted, there are no more
                            else {
                                break;
                            }
                        }
                    }
                    // Otherwise we wait for the next event using a wait_for (with a small offset for greater accuracy)
                    // Either that or until we get interrupted with a new event
                    else {
                        wait.wait_until(lock, next - waitOffset);
                    }
                }
                // Otherwise we wait for something to happen
                else {
                    wait.wait(lock);
                }
            });

        }
    }
}
