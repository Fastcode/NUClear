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

#ifndef NUCLEAR_DSL_WORD_ALWAYS_HPP
#define NUCLEAR_DSL_WORD_ALWAYS_HPP

#include "nuclear_bits/util/get_identifier.hpp"

namespace NUClear {
namespace dsl {
	namespace word {

		/**
		 * @brief Run the given task continously until shutdown.
		 *
		 * @details This task will start when the system starts up and continue to execute continually until the
		 *          whole system is told to shut down. The task will execute in its own unique thread rather than
		 *          the thread pool. However, if a task that it tries to execute is rescheduled such as with a Sync
		 *          the task will then be moved into the thread pool. When using this keyword you should endeavor
		 *          not to keep the system in a loop. Instead of putting a while(true) or similar loop in this
		 *          reaction, instead let it finish and restart itself. This allows it to terminate properly when
		 *          the system is shutdown. If it does not the system will hang when trying to terminate.
		 *          If this function is performing a blocking operation, try to make it interruptable with an
		 *          on<Shutdown> reaction so that the program can be cleanly terminated.
		 */
		struct Always {

			template <typename DSL, typename TFunc>
			static inline threading::ReactionHandle bind(Reactor& reactor, const std::string& label, TFunc&& callback) {

				// Get our identifier string
				std::vector<std::string> identifier =
					util::get_identifier<typename DSL::DSL, TFunc>(label, reactor.reactorName);

				auto unbinder = [](threading::Reaction& r) { r.enabled = false; };

				// Create our reaction and store it in the TypeCallbackStore
				auto reaction = std::make_shared<threading::Reaction>(
					reactor, std::move(identifier), std::forward<TFunc>(callback), std::move(unbinder));
				threading::ReactionHandle handle(reaction);

				// A lambda that will get a reaction task
				auto run = [reaction] {
					// Get a task
					auto task = reaction->getTask();

					// If we got a real task back
					if (task) {
						task = task->run(std::move(task));
					}
				};

				// This is our function that runs forever until the powerplant exits
				auto loop = [&reactor, run] {
					while (reactor.powerplant.running()) {
						try {
							run();
						}
						catch (...) {
						}
					}
				};

				reactor.powerplant.addThreadTask(loop);

				// Return our handle
				return handle;
			}
		};

	}  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_ALWAYS_HPP
