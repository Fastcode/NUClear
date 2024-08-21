/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_DSL_WORD_OPTIONAL_HPP
#define NUCLEAR_DSL_WORD_OPTIONAL_HPP

#include "../../threading/Reaction.hpp"
namespace NUClear {
namespace dsl {
    namespace word {

        template <typename T>
        struct OptionalWrapper {

            explicit OptionalWrapper(T&& d) : d(std::move(d)) {}

            T operator*() const {
                return std::move(d);
            }

            operator bool() const {
                return true;
            }

            T d;
        };

        /**
         * This is used to signify any optional properties in the DSL request.
         *
         * @code on<Trigger<T1>, Optional<With<T2>() @endcode
         * During system runtime, optional data does not need to be present when initialising a reaction system.
         * In the case above, when T1 is emitted to the system, the associated task will be queued for execution.
         * Should T2 be available, read-only access to the most recent emission of T2 will be provided to the
         * subscribing reaction.
         * However, should T2 not be present, the task will run without a reference to this data.
         *
         * This word is a modifier, and should be used to modify any "Get" DSL word.
         *
         *@par Implements
         *  Modification
         *
         * @tparam DSLWords The DSL word/activity being modified.
         */
        template <typename... DSLWords>
        struct Optional : Fusion<DSLWords...> {

        private:
            template <typename... T, int... Index>
            // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved) the elements in it are moved
            static auto wrap(std::tuple<T...>&& data, util::Sequence<Index...> /*s*/)
                -> decltype(std::make_tuple(OptionalWrapper<T>(std::move(std::get<Index>(data)))...)) {
                return std::make_tuple(OptionalWrapper<T>(std::move(std::get<Index>(data)))...);
            }

        public:
            template <typename DSL>
            static auto get(threading::ReactionTask& task)
                -> decltype(wrap(
                    Fusion<DSLWords...>::template get<DSL>(task),
                    util::GenerateSequence<
                        0,
                        std::tuple_size<decltype(Fusion<DSLWords...>::template get<DSL>(task))>::value>())) {

                // Wrap all of our data in optional wrappers
                return wrap(Fusion<DSLWords...>::template get<DSL>(task),
                            util::GenerateSequence<
                                0,
                                std::tuple_size<decltype(Fusion<DSLWords...>::template get<DSL>(task))>::value>());
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_OPTIONAL_HPP
