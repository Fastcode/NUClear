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

#ifndef NUCLEAR_LOGLEVEL_HPP
#define NUCLEAR_LOGLEVEL_HPP

// Why do we need to include platform.hpp here?
// Because windows defines a bunch of things for legacy reasons, one of which is a #define for ERROR as blank
// Of course this causes a problem when we define our own token below as error as the preprocessor removes it
// The platform.hpp fixes this (and many other windows nonsense) so include it here to solve that issue
#include "util/platform.hpp"

namespace NUClear {

/**
 * @brief LogLevel defines the different levels log messages can be set to.
 *
 * @details
 *  Log levels are used to provide different levels of detail on a per-reactor basis.
 *  The logging level of a reactor can be changed by setting it in the install function.
 */
enum LogLevel {
    /**
     * @brief
     *  The Trace level contains messages that are used to trace the exact flow of execution.
     *
     * @details
     *  This level is extremely verbose and often has a message per line of code.
     */
    TRACE,

    /**
     * @brief
     *  Debug contains messages that represent the inputs and outputs of different
     * computation units.
     *
     * @details
     *  If you have a function that performs three steps to do something
     *  then it's likely that you will have a message for the input and output of those
     *  three steps. Additionally you would likely have messages that check if it hit
     *  different branches.
     */
    DEBUG,

    /**
     * @brief
     *  The info level is used to provide high level goal messages such as function start
     *  or successful completion.
     *
     * @details
     *  This shows when key user-facing functionality is executed and tells us that everything
     *  is working without getting into the details.
     */
    INFO,

    /**
     * @brief The warning level is used to notify us that everything might not be working perfectly.
     *
     * @details
     *  Warnings are errors or inconsistencies that aren't fatal and generally do not completely
     *  break the system. However a warning message should require action from someone and should
     *  point to a section of the system that needs attention.
     */
    WARN,

    /**
     * @brief
     *  The error level is used to report unexpected behavior.

     * @details
     *  This level doesn't need to prefix a program-crashing issue but should be used to report major
     *  unexpected branches in logic or other constraint breaking problems such as failed assertions.
     *  All errors should require action from someone and should be addressed immediately.
     */
    ERROR,

    /**
     * @brief Fatal is a program destroying error that needs to be addressed immediately.
     *
     * @details
     *  If a fatal message is sent it should point to something that should never ever happen and
     *  ideally provide as much information as possible as to why it crashed. Fatal messages
     *  require action immediately and should always be addressed.
     */
    FATAL
};

}  // namespace NUClear

#endif  // NUCLEAR_LOGLEVEL_HPP
