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

#ifndef NUCLEAR_DSL_WORD_WITH_HPP
#define NUCLEAR_DSL_WORD_WITH_HPP

#include "nuclear_bits/dsl/Fusion.hpp"

#include "nuclear_bits/dsl/operation/CacheGet.hpp"

namespace NUClear {
namespace dsl {
	namespace word {

		/**
		 * @brief			Used to define any secondary data which should be provided to
		 *						a subscribing a reaction.
		 *
		 * @details		This is designed to be used in conjunction with the DSL request
		 *						for an on statement:
		 *						@code on<Trigger<T, ...>, With<TWiths, ...>>() @endcode
		 * 						When TWiths is emitted into the system, it will <b>not</b>
		 *						trigger a callback to the triggering unit. When T is emitted
		 *						into the system, read-only access to the most recent copy of
		 *						both T and TWiths will be provided via a callback to the
		 *						triggering unit.
		 *
		 * @attention QForTRENT:  --what if a copy of the data is not present at the
		 *						time of the required callback?  Does this stop the Task from
		 *						running?  Does this also work with Every and Always?
		 *
		 * @tparam 		TWiths the datatypes which will be retreived from the cache and
		 *						used in the reaction callback.
		 */
		template <typename... TWiths>
		struct With : public Fusion<operation::CacheGet<TWiths>...> {};

	}  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_WITH_HPP
