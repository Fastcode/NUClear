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

#ifndef NUCLEAR_DSL_WORD_OPTIONAL_HPP
#define NUCLEAR_DSL_WORD_OPTIONAL_HPP

namespace NUClear {
namespace dsl {
    namespace word {

        template <typename T>
        struct OptionalWrapper {

            OptionalWrapper(T&& d) : d(std::forward<T>(d)) {}

            T operator*() const {
                return std::move(d);
            }

            operator bool() const {
                return true;
            }

            T d;
        };

        /**
         * @brief
         *  This is used to signify any optional properties in the DSL request.
         *
         * @details
         *  @code on<Trigger<T1>, Optional<With<T2>() @endcode
         *  During system runtime, optional data does not need to be present when initialising a reaction within the
         *  system.  In the case above, when T1 is emitted to the system, the associated task will be queued for
         *  execution. Should T2 be available, read-only access to the most recent emission of T2 will be provided to
         *  the subscribing reaction.  However, should T2 not be present, the task will run without a reference to
         *  this data.
         *
         *  This word is a modifier, and should  be used to modify any "Get" DSL word.
         *
         *@par Implements
         *  Modification
         *
         * @tparam  DSLWords
         *  the DSL word/activity being modified.
         */
        template <typename... DSLWords>
        struct Optional : public Fusion<DSLWords...> {

        private:
            template <typename... T, int... Index>
            static inline auto wrap(std::tuple<T...>&& data, util::Sequence<Index...>)
                -> decltype(std::make_tuple(OptionalWrapper<T>(std::move(std::get<Index>(data)))...)) {
                return std::make_tuple(OptionalWrapper<T>(std::move(std::get<Index>(data)))...);
            }

        public:
            template <typename DSL>
            static inline auto get(threading::Reaction& r)
                -> decltype(wrap(Fusion<DSLWords...>::template get<DSL>(r),
                                 util::GenerateSequence<0,
                                                        std::tuple_size<decltype(
                                                            Fusion<DSLWords...>::template get<DSL>(r))>::value>())) {

                // Wrap all of our data in optional wrappers
                return wrap(Fusion<DSLWords...>::template get<DSL>(r),
                            util::GenerateSequence<0,
                                                   std::tuple_size<decltype(
                                                       Fusion<DSLWords...>::template get<DSL>(r))>::value>());
            }
        };

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_OPTIONAL_HPP
