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

#ifndef NUCLEAR_DSL_WORD_LAST_HPP
#define NUCLEAR_DSL_WORD_LAST_HPP

#include <list>
#include <type_traits>

#include "../../dsl/trait/is_transient.hpp"
#include "../../threading/Reaction.hpp"
#include "../../util/MergeTransient.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * A class that stores the last received items from a reaction
         *
         * This class stores the received items and provides conversion operators so that when passed into the reaction
         * it can be converted to an appropriate type.
         *
         * @tparam n The number of items to store.
         * @tparam T The type of the items to store.
         */
        template <size_t n, typename T>
        struct LastItemStorage {
            LastItemStorage() = default;

            /**
             * Constructs a LastItemStorage object with the given data.
             *
             * @param data The data to store.
             */
            explicit LastItemStorage(T&& data) : list({std::move(data)}) {}
            /**
             * Constructs a LastItemStorage object with the given data.
             *
             * @param data The data to store.
             */
            explicit LastItemStorage(const T& data) : list({data}) {}

            /**
             * Converts the stored list to a list of the given type.
             *
             * @tparam Output The type of the output list.
             *
             * @return The output list.
             */
            template <typename Output>
            operator std::list<Output>() const {

                std::list<Output> out;

                for (const T& item : list) {
                    out.push_back(Output(item));
                }

                return out;
            }

            /**
             * Converts the stored list to a vector of the given type.
             *
             * @tparam Output The type of the output vector.
             *
             * @return The output vector.
             */
            template <typename Output>
            operator std::vector<Output>() const {

                std::vector<Output> out;

                for (const T& item : list) {
                    out.push_back(Output(item));
                }

                return out;
            }

            /**
             * Bool operator to allow the reaction to decide not to run if there is no data.
             *
             * @return true     If the list is not empty.
             * @return false    If the list is empty.
             */
            operator bool() const {
                return !list.empty();
            }

            /// The items we are storing
            std::list<T> list;
        };

        /**
         * This instructs the Reactor to store the last n messages to a cache and provide read-only access to the
         * subscribing reaction.
         *
         *
         * @code on<Last<n, Trigger<T, ...>>>() @endcode
         * During system runtime, the PowerPlant will keep a record of the last [0-n] messages which were provided to
         * the subscribing reaction.
         * This list is ordered such that the oldest element is first, and the newest element is last.
         * Once n messages are stored, the trigger of a new reaction task will cause the newest copy to be appended to
         * the list, and the oldest copy to be dropped.
         *
         * This word is a modifier, and should be used to modify any "Get" DSL word.
         *
         * @par Multiple Statements
         *  @code on<Last<n, Trigger<T1>, With<T2>>() @endcode
         *  When applying this modifier to multiple get statements, a list will be returned for each statement.
         *  In this example, a list of up to n references for T1 and another list of up to n references for T2 will be
         *  provided to the subscribing reaction.
         *
         * @par Get Statements
         *  @code on<Last<n, Trigger<T1>, With<T2>>() @endcode
         *  When applied to a request containing a pure "Get" statement (such as the With statement), the data in the
         *  associated list will reference that which was available whenever the reaction was triggered.
         *  That is, the list for T2 may not represent the last n emissions of the data, but rather, only the data which
         *  was available at the time of generating the last n tasks.
         *
         * @par IO Keywords
         *  @code on<Last<n, Network<T>>>() @endcode
         *  When working with a DSL word that returns more than one item (such as the I/O Keywords), a list for each
         *  item will be returned.
         *  In the example above, a list of up to n references for port addresses, and another list of up to n
         *  references for T will be returned.
         *
         * @par Implements
         *  Modification
         *
         * @tparam  n        The number of records to be stored in the cache.
         * @tparam  DSLWords The DSL word/activity being modified.
         */
        template <size_t n, typename... DSLWords>
        struct Last : Fusion<DSLWords...> {

        private:
            template <typename... T, int... Index>
            // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved) the elements in it are moved
            static auto wrap(std::tuple<T...>&& data, util::Sequence<Index...> /*s*/)
                -> decltype(std::make_tuple(LastItemStorage<n, T>(std::move(std::get<Index>(data)))...)) {
                return std::make_tuple(LastItemStorage<n, T>(std::move(std::get<Index>(data)))...);
            }

        public:
            template <typename DSL>
            static auto get(threading::ReactionTask& task)
                -> decltype(wrap(
                    Fusion<DSLWords...>::template get<DSL>(task),
                    util::GenerateSequence<
                        0,
                        std::tuple_size<decltype(Fusion<DSLWords...>::template get<DSL>(task))>::value>())) {

                // Wrap all of our data in last list wrappers
                return wrap(Fusion<DSLWords...>::template get<DSL>(task),
                            util::GenerateSequence<
                                0,
                                std::tuple_size<decltype(Fusion<DSLWords...>::template get<DSL>(task))>::value>());
            }
        };

    }  // namespace word

    namespace trait {

        template <size_t n, typename T>
        struct is_transient<word::LastItemStorage<n, T>> : std::true_type {};

    }  // namespace trait
}  // namespace dsl

namespace util {

    template <size_t n, typename T>
    struct MergeTransients<dsl::word::LastItemStorage<n, T>> {
        static bool merge(dsl::word::LastItemStorage<n, T>& t, dsl::word::LastItemStorage<n, T>& d) {

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
