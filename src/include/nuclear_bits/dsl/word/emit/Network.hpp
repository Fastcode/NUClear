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

#ifndef NUCLEAR_DSL_WORD_EMIT_NETWORK_HPP
#define NUCLEAR_DSL_WORD_EMIT_NETWORK_HPP

#include <array>

#include "nuclear_bits/util/serialise/Serialise.hpp"

namespace NUClear {
namespace dsl {
	namespace word {
		namespace emit {

			/**
			 * @brief Holds a serialised packet to be sent over the network
			 *
			 * @details This data struct is used by the NetworkController to send data over the network.
			 */
			struct NetworkEmit {
				NetworkEmit() : target(""), hash(), payload(), reliable(false) {}

				/// The target to send this serialised packet to
				std::string target;
				/// The hash identifying the type of object
				std::array<uint64_t, 2> hash;
				/// The serialised data
				std::vector<char> payload;
				/// If the message should be sent reliably
				bool reliable;
			};

			/**
			 * @brief Emit over the NUClear network.
			 *
			 * @details Network emit uses the NUClear network in order to send messages to other NUClear systems.
			 *          The data can be sent by name to other systems or to all systems connected to the NUClear
			 *          network. These messages can be sent using either an unreliable protocol that does not
			 *          guarantee delivery, or using a reliable protocol that does. Note that if the target system
			 *          is not connected to the network, the send will be ignored even reliable is enabled.
			 *
			 * @param data      the data to emit
			 * @param target    the name of the system to send to, or empty for all systems. defaults to all.
			 *                  (an empty string). Optional
			 * @param reliable  true if the delivery of the message should be guaranteed. defaults to false.
			 *                  Optional
			 *
			 * @tparam DataType the type of the data to send
			 */
			template <typename DataType>
			struct Network {

				static void emit(PowerPlant& powerplant,
								 std::shared_ptr<DataType> data,
								 std::string target = "",
								 bool reliable		= false) {

					auto e = std::make_unique<NetworkEmit>();

					e->target   = target;
					e->hash		= util::serialise::Serialise<DataType>::hash();
					e->payload		= util::serialise::Serialise<DataType>::serialise(*data);
					e->reliable = reliable;

					powerplant.emit<Direct>(e);
				}

				static void emit(PowerPlant& powerplant, std::shared_ptr<DataType> data, bool reliable) {
					emit(powerplant, data, "", reliable);
				}
			};

		}  // namespace emit
	}	  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_NETWORK_HPP
