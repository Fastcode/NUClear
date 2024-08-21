/*
 * MIT License
 *
 * Copyright (c) 2023 NUClear Contributors
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

#ifndef NUCLEAR_THREADING_REACTION_IDENTIFIERS_HPP
#define NUCLEAR_THREADING_REACTION_IDENTIFIERS_HPP

#include <string>

namespace NUClear {
namespace threading {

    /**
     * This struct holds string fields that can be used to identify a reaction.
     */
    struct ReactionIdentifiers {

        /**
         * Construct a new Identifiers object.
         *
         * @param name     The name of the reaction provided by the user
         * @param reactor  The name of the reactor that this reaction belongs to
         * @param dsl      The DSL that this reaction was created from
         * @param function The callback function that this reaction is bound to
         */
        ReactionIdentifiers(std::string name, std::string reactor, std::string dsl, std::string function)
            : name(std::move(name)), reactor(std::move(reactor)), dsl(std::move(dsl)), function(std::move(function)) {}

        /// The name of the reaction provided by the user
        std::string name;
        /// The name of the reactor that this reaction belongs to
        std::string reactor;
        /// The DSL that this reaction was created from
        std::string dsl;
        /// The callback function that this reaction is bound to
        std::string function;
    };

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_REACTION_IDENTIFIERS_HPP
