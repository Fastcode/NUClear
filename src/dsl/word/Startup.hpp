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

#ifndef NUCLEAR_DSL_WORD_STARTUP_HPP
#define NUCLEAR_DSL_WORD_STARTUP_HPP

#include "../operation/TypeBind.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * @brief
         *  This is used to specify reactions which should occur at startup.
         *
         * @details
         *  @code on<Startup>() @endcode
         *  Any reactions listed with this DSL word will run directly after all reactors have been installed into the
         *  PowerPlant but before the system starts the main execution phase.  This is the only time these reactions
         *  will run.
         *
         *  Note that this request is generally used by reactor's which require information provided by another
         *  reactor's constructor.
         *
         * @par Implements
         *  Bind
         */
        struct Startup : public operation::TypeBind<Startup> {};

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_STARTUP_HPP
