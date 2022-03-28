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

#ifndef NUCLEAR_DSL_FUSION_NOOP_HPP
#define NUCLEAR_DSL_FUSION_NOOP_HPP

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
            static inline void bind(const std::shared_ptr<threading::Reaction>&, Args...) {}

            template <typename DSL>
            static inline std::tuple<> get(threading::Reaction&) {
                return std::tuple<>();
            }

            template <typename DSL>
            static inline bool precondition(threading::Reaction&) {
                return true;
            }

            template <typename DSL>
            static inline int priority(threading::Reaction&) {
                return word::Priority::NORMAL::value;
            }

            template <typename DSL>
            static inline std::unique_ptr<threading::ReactionTask> reschedule(
                std::unique_ptr<threading::ReactionTask>&& task) {
                return std::move(task);
            }

            template <typename DSL>
            static inline void postcondition(threading::ReactionTask&) {}
        };

        /**
         * @brief Struct to act as Parsed DSL statement that does nothing. Used as a placeholder to satisfy some
         *        dead template branches that are required for the code to compile.
         */
        struct ParsedNoOp {
            struct DSL {};

            static inline std::tuple<> bind(const std::shared_ptr<threading::Reaction>&);

            static inline std::tuple<> get(threading::Reaction&);

            static inline bool precondition(threading::Reaction&);

            static inline int priority(threading::Reaction&);

            static inline std::unique_ptr<threading::ReactionTask> reschedule(
                std::unique_ptr<threading::ReactionTask>&& task);

            static inline void postcondition(threading::ReactionTask&);
        };

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_NOOP_HPP
