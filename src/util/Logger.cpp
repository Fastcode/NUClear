/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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

#include "Logger.hpp"

#include <memory>
#include <string>

#include "../Reactor.hpp"
#include "../dsl/word/emit/Inline.hpp"
#include "../message/LogMessage.hpp"
#include "../threading/ReactionTask.hpp"

namespace NUClear {
namespace util {

    namespace {  // Anonymous namespace for internal linkage

        /**
         * Get the current reactor that is running based on the passed in reactor, or the current task if not in a
         * reactor.
         *
         * @param calling_reactor The reactor that is calling this function or nullptr if not called from a reactor
         *
         * @return The current reactor or nullptr if not in a reactor
         */
        const Reactor* get_current_reactor(const Reactor* calling_reactor) {
            if (calling_reactor != nullptr) {
                return calling_reactor;
            }
            // Get the current task
            const auto* const current_task = threading::ReactionTask::get_current_task();
            return current_task != nullptr && current_task->parent != nullptr ? &current_task->parent->reactor
                                                                              : nullptr;
        }

    }  // namespace

    Logger::LogLevels Logger::get_current_log_levels(const Reactor* calling_reactor) {
        // Get the current reactor either from the passed in reactor or the current task
        const auto* reactor = get_current_reactor(calling_reactor);
        return reactor != nullptr ? LogLevels(reactor->log_level, reactor->min_log_level)
                                  : LogLevels(LogLevel::UNKNOWN, LogLevel::UNKNOWN);
    }

    void Logger::do_log(const Reactor* calling_reactor, const LogLevel& level, const std::string& message) {
        // Get the current context
        const auto* reactor      = get_current_reactor(calling_reactor);
        const auto* current_task = threading::ReactionTask::get_current_task();
        auto log_levels          = get_current_log_levels(reactor);

        // Inline emit the log message to default handlers to pause the current task until the log message is processed
        powerplant.emit<dsl::word::emit::Inline>(
            std::make_unique<message::LogMessage>(level,
                                                  log_levels.display_log_level,
                                                  message,
                                                  reactor != nullptr ? reactor->reactor_name : "",
                                                  current_task != nullptr ? current_task->statistics : nullptr));
    }
}  // namespace util
}  // namespace NUClear
