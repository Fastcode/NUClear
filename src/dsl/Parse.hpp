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

#ifndef NUCLEAR_DSL_PARSE_HPP
#define NUCLEAR_DSL_PARSE_HPP

#include "Fusion.hpp"
#include "validation/Validation.hpp"

namespace NUClear {
namespace dsl {

    template <typename... Sentence>
    struct Parse {

        using DSL = Fusion<Sentence...>;

        template <typename... Arguments>
        static inline auto bind(const std::shared_ptr<threading::Reaction>& r, Arguments... args)
            -> decltype(DSL::template bind<Parse<Sentence...>>(r, std::forward<Arguments>(args)...)) {
            return DSL::template bind<Parse<Sentence...>>(r, std::forward<Arguments>(args)...);
        }

        static inline auto get(threading::Reaction& r) -> decltype(
            std::conditional_t<fusion::has_get<DSL>::value, DSL, fusion::NoOp>::template get<Parse<Sentence...>>(r)) {
            return std::conditional_t<fusion::has_get<DSL>::value, DSL, fusion::NoOp>::template get<Parse<Sentence...>>(
                r);
        }

        static inline bool precondition(threading::Reaction& r) {
            return std::conditional_t<fusion::has_precondition<DSL>::value, DSL, fusion::NoOp>::template precondition<
                Parse<Sentence...>>(r);
        }

        static inline int priority(threading::Reaction& r) {
            return std::conditional_t<fusion::has_priority<DSL>::value, DSL, fusion::NoOp>::template priority<
                Parse<Sentence...>>(r);
        }

        static std::unique_ptr<threading::ReactionTask> reschedule(std::unique_ptr<threading::ReactionTask>&& task) {
            return std::conditional_t<fusion::has_reschedule<DSL>::value, DSL, fusion::NoOp>::template reschedule<DSL>(
                std::move(task));
        }

        static inline void postcondition(threading::ReactionTask& r) {
            std::conditional_t<fusion::has_postcondition<DSL>::value, DSL, fusion::NoOp>::template postcondition<
                Parse<Sentence...>>(r);
        }
    };

}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_PARSE_HPP
