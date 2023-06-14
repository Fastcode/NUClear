/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#include "PowerPlant.hpp"

// See https://valgrind.org/docs/manual/drd-manual.html#drd-manual.CXX11
#if defined(USE_VALGRIND) && !defined(NDEBUG)
// NOLINTBEGIN
namespace std {
extern "C" {
static void* execute_native_thread_routine(void* __p) {
    thread::_State_ptr __t{static_cast<thread::_State*>(__p)};
    __t->_M_run();
    return nullptr;
}
void thread::_M_start_thread(_State_ptr state, void (*depend)()) {
    // Make sure it's not optimized out, not even with LTO.
    asm("" : : "rm"(depend));

    if (!__gthread_active_p()) {
    #if __cpp_exceptions
        throw system_error(make_error_code(errc::operation_not_permitted), "Enable multithreading to use std::thread");
    #else
        __builtin_abort();
    #endif
    }

    const int err = __gthread_create(&_M_id._M_thread, &execute_native_thread_routine, state.get());
    if (err)
        __throw_system_error(err);
    state.release();
}
}
}  // namespace std
// NOLINTEND
#endif  // defined(USE_VALGRIND) && !defined(NDEBUG)

namespace NUClear {

PowerPlant* PowerPlant::powerplant = nullptr;  // NOLINT

PowerPlant::~PowerPlant() {

    // Bye bye powerplant
    powerplant = nullptr;
}

void PowerPlant::start() {

    // We are now running
    is_running.store(true);

    // Direct emit startup event
    emit<dsl::word::emit::Direct>(std::make_unique<dsl::word::Startup>());

    // Start all of the threads
    scheduler.start(configuration.thread_count);
}

void PowerPlant::submit(std::unique_ptr<threading::ReactionTask>&& task) {
    scheduler.submit(std::move(task));
}

void PowerPlant::shutdown() {

    // Stop running before we emit the Shutdown event
    // Some things such as on<Always> depend on this flag and it's possible to miss it
    is_running.store(false);

    // Emit our shutdown event
    emit(std::make_unique<dsl::word::Shutdown>());

    // Shutdown the scheduler
    scheduler.shutdown();
}

bool PowerPlant::running() const {
    return is_running.load();
}
}  // namespace NUClear
