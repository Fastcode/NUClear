/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
 *
 * This file is part of the NUClear codebase.
 * See https://github.com/Fastcode/NUClear for further info.
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
#include "fusion/NoOp.hpp"
#include "validation/Validation.hpp"

namespace NUClear {
namespace dsl {

    template <typename... Sentence>
    struct Parse {

        using DSL = Fusion<Sentence..., fusion::NoOp>;

        template <typename... Arguments>
        static auto bind(const std::shared_ptr<threading::Reaction>& r, Arguments&&... args)
            -> decltype(DSL::template bind<Parse<Sentence...>>(r, std::forward<Arguments>(args)...)) {
            return DSL::template bind<Parse<Sentence...>>(r, std::forward<Arguments>(args)...);
        }

        static auto get(threading::ReactionTask& task) -> decltype(DSL::template get<Parse<Sentence...>>(task)) {
            return DSL::template get<Parse<Sentence...>>(task);
        }

        static auto group(threading::ReactionTask& task) -> decltype(DSL::template group<Parse<Sentence...>>(task)) {
            return DSL::template group<Parse<Sentence...>>(task);
        }

        static auto pool(threading::ReactionTask& task) -> decltype(DSL::template pool<Parse<Sentence...>>(task)) {
            return DSL::template pool<Parse<Sentence...>>(task);
        }

        static auto postrun(threading::ReactionTask& task)
            -> decltype(DSL::template postrun<Parse<Sentence...>>(task)) {
            return DSL::template postrun<Parse<Sentence...>>(task);
        }

        static auto prerun(threading::ReactionTask& task) -> decltype(DSL::template prerun<Parse<Sentence...>>(task)) {
            return DSL::template prerun<Parse<Sentence...>>(task);
        }

        static auto precondition(threading::ReactionTask& task)
            -> decltype(DSL::template precondition<Parse<Sentence...>>(task)) {
            return DSL::template precondition<Parse<Sentence...>>(task);
        }

        static auto priority(threading::ReactionTask& task)
            -> decltype(DSL::template priority<Parse<Sentence...>>(task)) {
            return DSL::template priority<Parse<Sentence...>>(task);
        }

        static auto run_inline(threading::ReactionTask& task)
            -> decltype(DSL::template run_inline<Parse<Sentence...>>(task)) {
            return DSL::template run_inline<Parse<Sentence...>>(task);
        }

        static auto scope(threading::ReactionTask& task) -> decltype(DSL::template scope<Parse<Sentence...>>(task)) {
            return DSL::template scope<Parse<Sentence...>>(task);
        }
    };

}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_PARSE_HPP
