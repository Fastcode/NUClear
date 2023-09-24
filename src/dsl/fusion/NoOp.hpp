/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#ifndef NUCLEAR_DSL_FUSION_NOOP_HPP
#define NUCLEAR_DSL_FUSION_NOOP_HPP

#include <typeindex>

#include "../../threading/Reaction.hpp"
#include "../../threading/ReactionTask.hpp"
#include "../../util/GroupDescriptor.hpp"
#include "../../util/ThreadPoolDescriptor.hpp"
#include "../word/Priority.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /**
         * @brief Struct to act as a DSL word that does nothing. Used as a placeholder to satisfy some dead template
         *        branches that are required for the code to compile.
         */
        struct NoOp {

            template <typename DSL, typename... Args>
            static inline void bind(const std::shared_ptr<threading::Reaction>& /*reaction*/, Args... /*args*/) {
                // Empty as this is a no-op placeholder
            }

            template <typename DSL>
            static inline std::tuple<> get(const threading::Reaction& /*reaction*/) {
                return {};
            }

            template <typename DSL>
            static inline bool precondition(const threading::Reaction& /*reaction*/) {
                return true;
            }

            template <typename DSL>
            static inline int priority(const threading::Reaction& /*reaction*/) {
                return word::Priority::NORMAL::value;
            }

            template <typename DSL>
            static inline util::GroupDescriptor group(const threading::Reaction& /*reaction*/) {
                return util::GroupDescriptor{};
            }

            template <typename DSL>
            static inline util::ThreadPoolDescriptor pool(const threading::Reaction& /*reaction*/) {
                return util::ThreadPoolDescriptor{};
            }

            template <typename DSL>
            static inline void postcondition(const threading::ReactionTask& /*task*/) {
                // Empty as this is a no-op placeholder
            }
        };

        /**
         * @brief Struct to act as Parsed DSL statement that does nothing. Used as a placeholder to satisfy some
         *        dead template branches that are required for the code to compile.
         */
        struct ParsedNoOp {
            struct DSL {};

            static std::tuple<> bind(const std::shared_ptr<threading::Reaction>&);

            static std::tuple<> get(threading::Reaction&);

            static bool precondition(threading::Reaction&);

            static int priority(threading::Reaction&);

            static util::GroupDescriptor group(threading::Reaction&);

            static util::ThreadPoolDescriptor pool(threading::Reaction&);

            static void postcondition(threading::ReactionTask&);
        };

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_NOOP_HPP
