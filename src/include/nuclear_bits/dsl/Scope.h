/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_SCOPE_H
#define NUCLEAR_DSL_SCOPE_H

namespace NUClear {
    namespace dsl {
        
        /**
         * @brief This struct contains the default emit types that are available
         *
         * @details
         *  When using the emit function, these options can be added in order to specify where the packet is to be
         *  sent. Multiple of these can be specified in order to emit to multiple destinations.
         */
        struct Scope {
            /// @brief Send data to reactions that request it, but it will bypass the threadpool, directy calling the function.
            struct DIRECT;
            /// @brief Send as ususal to reactions that request it within this powerplant.
            struct LOCAL;
            /// @brief Send data to other powerplants over the network but not this one.
            struct NETWORK;
            /**
             * @brief Emit after all reactors have been installed directly before the main threadpool starts.
             *
             * @attention
             *  If INITIALIZE is used after startup, it will result in no action. It will also store the event resulting
             * in a memory leak.
             */
            struct INITIALIZE;
        };
    }
}

#endif
