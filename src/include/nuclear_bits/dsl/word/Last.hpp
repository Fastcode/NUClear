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

#include <type_traits>
#include <list>
#include "nuclear_bits/util/MergeTransient.hpp"

namespace NUClear {
namespace dsl {
	namespace word {

		template <size_t len, typename TData>
		struct LastItemStorage {
			// The items we are storing
			std::list<TData> list;

			LastItemStorage() : list() {}

			LastItemStorage(TData&& data) : list({data}) {}

			template <typename TOutput>
			operator std::list<TOutput>() const {

				std::list<TOutput> out;

				for (const TData& item : list) {
					out.push_back(TOutput(item));
				}

				return out;
			}

			template <typename TOutput>
			operator std::vector<TOutput>() const {

				std::vector<TOutput> out;

				for (const TData& item : list) {
					out.push_back(TOutput(item));
				}

				return out;
			}

			operator bool() const {
				return !list.empty();
			}
		};

		template <size_t len, typename... DSLWords>
		struct Last : public Fusion<DSLWords...> {

		private:
			template <typename... TData, int... Index>
			static inline auto wrap(std::tuple<TData...>&& data, util::Sequence<Index...>)
				-> decltype(std::make_tuple(LastItemStorage<len, TData>(std::move(std::get<Index>(data)))...)) {
				return std::make_tuple(LastItemStorage<len, TData>(std::move(std::get<Index>(data)))...);
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

		template <size_t len, typename TData>
		struct is_transient<word::LastItemStorage<len, TData>> : public std::true_type {};

	}  // namespace trait
}  // namespace dsl

namespace util {

	template <size_t len, typename T>
	struct MergeTransients<dsl::word::LastItemStorage<len, T>> {
		static inline bool merge(dsl::word::LastItemStorage<len, T>& t, dsl::word::LastItemStorage<len, T>& d) {

			// We put the contents from data into the transient storage list
			t.list.insert(t.list.end(), d.list.begin(), d.list.end());

			// Then truncate the old transient data
			if (t.list.size() > len) {
				t.list.erase(t.list.begin(), std::next(t.list.begin(), t.list.size() - len));
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
