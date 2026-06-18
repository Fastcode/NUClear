/*
 * MIT License
 *
 * Copyright (c) 2026 NUClear Contributors
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

#include "IOController.hpp"

#include "../threading/ReactionTask.hpp"

namespace NUClear {
namespace extension {

    void IOController::register_inline_bump_reactions() {
        on<Trigger<dsl::word::IOConfiguration>, Inline::ALWAYS>().then("Configure IO bump", [this] { bump(); });
        on<Trigger<dsl::word::IOFinished>, Inline::ALWAYS>().then("IO Finished bump", [this] { bump(); });
        on<Trigger<dsl::operation::Unbind<IO>>, Inline::ALWAYS>().then("Unbind IO bump", [this] { bump(); });
        on<Shutdown, Inline::ALWAYS>().then("Shutdown IO bump", [this] { bump(); });
    }

    void IOController::register_shutdown_control() {
        on<Shutdown, Pool<IOPool>, Priority::HIGH, Inline::NEVER>().then("Shutdown IO Controller", [this] {
            running.store(false, std::memory_order_release);
        });
    }

    bool IOController::prepare_poll_iteration() {
        if (!running.load(std::memory_order_acquire)) {
            return false;
        }

        if (dirty.load(std::memory_order_acquire)) {
            rebuild_list();
        }

        return true;
    }

    void IOController::resubmit_poll_task() {
        powerplant.submit(threading::ReactionTask::get_current_task()->parent->get_task());
    }

    void IOController::register_poll_loop(std::function<void()> wait_and_process) {
        on<Startup, Pool<IOPool>, Priority::NORMAL, Inline::NEVER>().then("IO Poll", [this, wait = std::move(wait_and_process)] {
            if (!prepare_poll_iteration()) {
                return;
            }

            wait();
            resubmit_poll_task();
        });
    }

}  // namespace extension
}  // namespace NUClear
