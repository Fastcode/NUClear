/**
 * Copyright (C) 2013 Jake Woods <jake.f.woods@gmail.com>, Trent Houliston <trent@houliston.me>
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
         * @brief This enum is used to modify in what way a packet is sent, either Local, Network or both
         *
         * @details
         *  When using the emit function, these options can be added in order to specify where the packet is to be
         *  sent. If nothing or LOCAL is specified, then it will only be send within this powerplant. If NETWORK is
         *  specified, then the data will be sent to other powerplants but not this one, if both LOCAL and NETWORK
         *  are chosen, then the event will propagate to both the network and local powerplants.
         */
        struct Scope {
            struct DIRECT;
            struct LOCAL;
            struct NETWORK;
            struct INITIALIZE;
        };
    }
}

#endif
