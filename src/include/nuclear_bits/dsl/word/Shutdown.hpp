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

#ifndef NUCLEAR_DSL_WORD_SHUTDOWN_HPP
#define NUCLEAR_DSL_WORD_SHUTDOWN_HPP

#include "nuclear_bits/dsl/operation/TypeBind.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This is used to specify any reactions/tasks which should occur at shutdown.
         *
         * @details
         *  Once the shudown command is emitted to the PowerPlant, all existing tasks within the system will complete
         *  their processing as per their current place in the queue.
         *
         *  Any reactions listed with this keyword will then be queued and processed.
         *  @code  on<Shutdown>() @endcode
         *
         *  Note that after the shutdown event is triggered, any other reactions/events which would normally occur
         *  based on system emissions will not be processed.  I.e; all tasks, baring the shutdown tasks will be ignored.
         *
         *  Once all tasks have finished processing, the system will terminate.
         *
         * @par Implements
         *  Bind
         *
         * @par TRENT????
         *  Normal Use:
         *  @code  on<Shutdown>() @endcode
         *  Is what I have written correct? If shutdown doesnt have a <type T> associated with it, what is the code to
         *  allow the system to know when to shutdonw?
         *  @code on<Trigger<T>, Shutdown>>() @endcode
         *  How many reactions can a specify for shutdown?  Can each reactor have thier own on<Shutdown> process?
         *  This page also requires aan example of it being used.
         */
        struct Shutdown : public operation::TypeBind<Shutdown> {};

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_SHUTDOWN_HPP
