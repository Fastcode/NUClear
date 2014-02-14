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

#ifndef NUCLEAR_DSL_EVERY_H
#define NUCLEAR_DSL_EVERY_H

namespace NUClear {
    namespace dsl {
        
        /**
         * @brief This type is used within an every in order to measure a frequency rather then a period.
         *
         * @author Trent Houliston
         */
        template <typename period>
        struct Per {
            Per() = delete;
            ~Per() = delete;
        };
        
        /**
         * @ingroup SmartTypes
         * @brief A special flag type which specifies that this reaction should trigger at the given rate
         *
         * @details
         *  This type is used in a Trigger statement to specify that the given reaction should trigger at the rate
         *  set by this type. For instance, if the type was specified as shown in the following example
         *  @code on<Trigger<Every<2, std::chrono::seconds>> @endcode
         *  then the callback would execute every 2 seconds. This type simply needs to exist in the trigger for the
         *  correct timing to be called.
         *
         * @attention Note that the period which is used to measure the ticks in must be equal to or greater then
         *  clock::duration or the program will not compile
         *
         * @tparam ticks the number of ticks of a paticular type to wait
         * @tparam period a type of duration (e.g. std::chrono::seconds) to measure the ticks in, defaults to clock duration.
         *                  This paramter may also be wrapped in a Per<> template in order to write a frequency rather then a period.
         *
         * @return Returns the time the every was emitted as an clock::time_point
         */
        template <int ticks, class period = NUClear::clock::duration>
        struct Every {
            Every() = delete;
            ~Every() = delete;
        };
    }
}

#endif
