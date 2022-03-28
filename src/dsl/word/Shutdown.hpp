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

#ifndef NUCLEAR_DSL_WORD_SHUTDOWN_HPP
#define NUCLEAR_DSL_WORD_SHUTDOWN_HPP

#include "../operation/TypeBind.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This is used to specify any reactions/tasks which should occur during shutdown.
         *
         * @details
         *  @code  on<Shutdown>() @endcode
         *  Once the shutdown command has been emitted to the PowerPlant, all existing tasks within the system will
         *  complete their processing as per their current place in the queue.
         *
         *  Any reactions listed with this keyword will then be queued and processed.  Tasks in this queue are ordered
         *  based on their priority level, then their emission timestamp.
         *
         *  After the shutdown event is triggered, any other reactions/events which would normally occur based on
         *  system emissions will not be processed. That is, all tasks baring the shutdown tasks will be ignored.
         *
         *  Once all Shutdown tasks have finished processing, the system will terminate.
         *
         * @attention
         *  An on<Shutdown>() request simply specifies a reaction/task which should run during the system shutdown
         *  process.  It is NOT the command which initialises the process.
         *
         * @par Implements
         *  Bind
         */
        struct Shutdown : public operation::TypeBind<Shutdown>, Priority::IDLE {};

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_SHUTDOWN_HPP
