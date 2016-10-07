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

#ifndef NUCLEAR_EXTENSION_CHRONOCONTROLLER
#define NUCLEAR_EXTENSION_CHRONOCONTROLLER

#include "nuclear"

namespace NUClear {
    namespace extension {

        class ChronoController : public Reactor {
        private:
            struct Step {
                Step() : jump(), next(), reactions() {}
                Step(const clock::duration& jump, const clock::time_point& next
                    , const std::vector<std::shared_ptr<threading::Reaction>>& reactions)
                : jump(jump)
                , next(next)
                , reactions(reactions) {}

                clock::duration jump;
                clock::time_point next;
                std::vector<std::shared_ptr<threading::Reaction>> reactions;

                bool operator< (const Step& other) const {
                    return next < other.next;
                }
            };

        public:
            explicit ChronoController(std::unique_ptr<NUClear::Environment> environment);

        private:
            std::vector<Step> steps;
            std::vector<std::shared_ptr<const dsl::word::emit::DelayEmit>> delayEmits;
            std::mutex mutex;
            std::condition_variable wait;
            
            NUClear::clock::duration waitOffset;
        };

    }  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_CHRONOCONTROLLER
