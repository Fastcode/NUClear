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

#ifndef NUCLEAR_DSL_WORD_TRIGGER_HPP
#define NUCLEAR_DSL_WORD_TRIGGER_HPP

#include "nuclear_bits/dsl/operation/CacheGet.hpp"
#include "nuclear_bits/dsl/operation/TypeBind.hpp"

namespace NUClear {
namespace dsl {
	namespace word {

		/**
		 * @brief		Used to request any data dependent reactions in the system.
		 *
		 * @details	This is designed to be used as a DSL request for an on statement:
		 * 					@code	on<Trigger<T, ...>>() @endcode
		 *					It will enact the execution a task whenever T is emitted into the
		 *					system.  When this occurs, read-only access to T will be provided
		 *					to the triggering unit via a callback.
		 *
		 *					The request can handle multiple types.  Note that when used for
		 *					multiple types, the reaction will only be triggered once
		 *					<b>all</b> of the types have been emitted (at least once) since the
		 *					last occurence of the event.
		 *
		 * @tparam 	T the datatypes on which a reaction callback will be triggered.
		 *					These will be flagged as <b>primary</b> datatype/s for the
		 *					subscribing reaction.
		 */
		template <typename T>
		struct Trigger : public operation::TypeBind<T>, public operation::CacheGet<T> {};

	}  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_TRIGGER_HPP
