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

#ifndef NUCLEAR_DSL_WORD_LAST_HPP
#define NUCLEAR_DSL_WORD_LAST_HPP

#include <list>
#include <type_traits>
#include "nuclear_bits/util/MergeTransient.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        template <size_t n, typename T>
        struct LastItemStorage {
            // The items we are storing
            std::list<T> list;

            LastItemStorage() : list() {}

            LastItemStorage(T&& data) : list({data}) {}

            template <typename Output>
            operator std::list<Output>() const {

                std::list<Output> out;

                for (const T& item : list) {
                    out.push_back(Output(item));
                }

                return out;
            }

            template <typename Output>
            operator std::vector<Output>() const {

                std::vector<Output> out;

                for (const T& item : list) {
                    out.push_back(Output(item));
                }

                return out;
            }

            operator bool() const {
                return !list.empty();
            }
        };

        /**
         * @brief
         *  This instructs the PowerPlant to store the last n emitted messages (of the associated type) to the
         *  cache and provide read-only access of all n instances to the subscribing reaction.
         *
         * @details
         *  @code on<Last<n, Trigger<T, ...>>>() @endcode
         *  During system runtime, the powerplant will keep a record of the last [0-n] emissions of this type. Once
         *  n emissions are stored, the emission of each new instance will cause the oldest copy to be dropped.
         *  When the task is triggered, read-only access to any messages currently in the cache will be provided to the
         *  subscribing reaction.
         *
         *  Note that this word is a modifier, and should only be used to modify any other "Get" DSL word.
         *
         * @attention
         *  @code on<Last<n, Trigger<T1>, With<T2>>() @endcode
         *  When applying this modifier to multiple get statements, a list will be returned for each get statement. In
         *  this example, a list of up to n references for T1 and another list of up to n references for T2 will be
         *  returned.
         *
         * @attention
         *  @code on<Last<n, Network<T>>>() @endcode
         *  When working with a DSL word that returns more than one item, a list for each item will be returned. In
         *  this example, a list of up to n references for port addresses, and another list of up to n references
         *  for T will be returned.
         *
         * @par Implements
         *  Modification
         *
         * @tparam  n
         *  the number of records to be stored in the cache
         * @tparam  DSLWords
         *  the DSL word/activity being modified
         */
        template <size_t n, typename... DSLWords>
        struct Last : public Fusion<DSLWords...> {

        private:
            template <typename... T, int... Index>
            static inline auto wrap(std::tuple<T...>&& data, util::Sequence<Index...>)
                -> decltype(std::make_tuple(LastItemStorage<n, T>(std::move(std::get<Index>(data)))...)) {
                return std::make_tuple(LastItemStorage<n, T>(std::move(std::get<Index>(data)))...);
            }

        public:
            template <typename DSL>
            static inline auto get(threading::Reaction& r)
                -> decltype(wrap(Fusion<DSLWords...>::template get<DSL>(r),
                                 util::GenerateSequence<0,
                                                        std::tuple_size<decltype(
                                                            Fusion<DSLWords...>::template get<DSL>(r))>::value>())) {

                // Wrap all of our data in last list wrappers
                return wrap(Fusion<DSLWords...>::template get<DSL>(r),
                            util::GenerateSequence<0,
                                                   std::tuple_size<decltype(
                                                       Fusion<DSLWords...>::template get<DSL>(r))>::value>());
            }
        };

    }  // namespace word

    namespace trait {

        template <size_t n, typename T>
        struct is_transient<word::LastItemStorage<n, T>> : public std::true_type {};

    }  // namespace trait
}  // namespace dsl

namespace util {

    template <size_t n, typename T>
    struct MergeTransients<dsl::word::LastItemStorage<n, T>> {
        static inline bool merge(dsl::word::LastItemStorage<n, T>& t, dsl::word::LastItemStorage<n, T>& d) {

            // We put the contents from data into the transient storage list
            t.list.insert(t.list.end(), d.list.begin(), d.list.end());

            // Then truncate the old transient data
            if (t.list.size() > n) {
                t.list.erase(t.list.begin(), std::next(t.list.begin(), t.list.size() - n));
            }

            // Finally clear the data list and put the transient list in it
            d.list.clear();
            d.list.insert(d.list.begin(), t.list.begin(), t.list.end());

            return true;
        };
    };

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_LAST_HPP
