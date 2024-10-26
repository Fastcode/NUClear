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

#ifndef NUCLEAR_DSL_FUSION_NO_OP_HPP
#define NUCLEAR_DSL_FUSION_NO_OP_HPP

#include <typeindex>

#include "../../threading/Reaction.hpp"
#include "../../threading/ReactionTask.hpp"
#include "../../util/GroupDescriptor.hpp"
#include "../../util/Inline.hpp"
#include "../../util/ThreadPoolDescriptor.hpp"
#include "../word/Pool.hpp"
#include "../word/Priority.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /**
         * Struct to act as a DSL word that does nothing.
         * Used as a placeholder to satisfy some dead template branches that are required for the code to compile.
         */
        struct NoOp {

            template <typename DSL, typename... Args>
            static void bind(const std::shared_ptr<threading::Reaction>& /*reaction*/, Args... /*args*/) {
                // Empty as this is a no-op placeholder
            }

            template <typename DSL>
            static std::tuple<> get(const threading::ReactionTask& /*task*/) {
                return {};
            }

            template <typename DSL>
            static std::set<std::shared_ptr<const util::GroupDescriptor>> group(
                const threading::ReactionTask& /*task*/) {
                return {};
            }

            template <typename DSL>
            static util::Inline run_inline(const threading::ReactionTask& /*task*/) {
                return util::Inline::NEUTRAL;
            }

            template <typename DSL>
            static bool precondition(const threading::ReactionTask& /*task*/) {
                return true;
            }

            template <typename DSL>
            static void post_run(const threading::ReactionTask& /*task*/) {
                // Empty as this is a no-op placeholder
            }

            template <typename DSL>
            static void pre_run(const threading::ReactionTask& /*task*/) {
                // Empty as this is a no-op placeholder
            }

            template <typename DSL>
            static int priority(const threading::ReactionTask& /*task*/) {
                return word::Priority::NORMAL::value;
            }

            template <typename DSL>
            static std::shared_ptr<const util::ThreadPoolDescriptor> pool(const threading::ReactionTask& /*task*/) {
                return word::Pool<>::descriptor();
            }

            template <typename DSL>
            static std::tuple<> scope(const threading::ReactionTask& /*task*/) {
                return {};
            }
        };

        /**
         * Struct to act as Parsed DSL statement that does nothing.
         * Used as a placeholder to satisfy some dead template branches that are required for the code to compile.
         */
        struct ParsedNoOp {
            struct DSL {};

            static std::tuple<> bind(const std::shared_ptr<threading::Reaction>&);

            static std::tuple<> get(threading::ReactionTask&);

            static std::set<std::shared_ptr<const util::GroupDescriptor>> group(threading::ReactionTask&);

            static util::Inline run_inline(threading::ReactionTask&);

            static bool precondition(threading::ReactionTask&);

            static void post_run(threading::ReactionTask&);

            static void pre_run(threading::ReactionTask&);

            static int priority(threading::ReactionTask&);

            static std::shared_ptr<const util::ThreadPoolDescriptor> pool(threading::ReactionTask&);

            static std::tuple<> scope(threading::ReactionTask&);
        };

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_NO_OP_HPP
