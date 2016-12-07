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

#include "nuclear_bits/threading/ReactionTask.hpp"
#include "nuclear_bits/threading/Reaction.hpp"

namespace NUClear {
namespace threading {

	// Initialize our id source
	std::atomic<uint64_t> ReactionTask::taskIdSource(0);

	// This here is a hack because for some reason Xcode is broken and doesn't generate the destructors for this
	// class...???!!!
	// TODO once XCode stops being so stupid, you can remove this
	message::ReactionStatistics r;

	// Initialize our current task
	ATTRIBUTE_TLS ReactionTask* ReactionTask::currentTask = nullptr;

	ReactionTask::ReactionTask(Reaction& parent,
							   int priority,
							   std::function<std::unique_ptr<ReactionTask>(std::unique_ptr<ReactionTask>&&)> callback)
		: parent(parent)
		, taskId(++taskIdSource)
		, priority(priority)
		, stats(new message::ReactionStatistics{parent.identifier,
												parent.reactionId,
												taskId,
												currentTask ? currentTask->parent.reactionId : 0,
												currentTask ? currentTask->taskId : 0,
												clock::now(),
												clock::time_point(std::chrono::seconds(0)),
												clock::time_point(std::chrono::seconds(0)),
												nullptr})
		, callback(callback) {

		// There is one new active task
		++parent.activeTasks;
	}

	const ReactionTask* ReactionTask::getCurrentTask() {
		return currentTask;
	}

	std::unique_ptr<ReactionTask> ReactionTask::run(std::unique_ptr<ReactionTask>&& us) {

		// Update our current task
		auto oldTask = currentTask;
		currentTask  = this;

		// Run our callback at catch the returned task (to see if it rescheduled itself)
		us = callback(std::move(us));

		// If we were not rescheduled then finish off our stats
		if (us) {
			// There is one less task
			--parent.activeTasks;
		}

		// Reset our task back
		currentTask = oldTask;

		// Return our original task
		return std::move(us);
	}
}
}
