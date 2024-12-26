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

#ifndef NUCLEAR_DSL_WORD_EMIT_NETWORK_HPP
#define NUCLEAR_DSL_WORD_EMIT_NETWORK_HPP

#include <array>
#include <utility>

#include "../../../util/serialise/Serialise.hpp"

namespace NUClear {
namespace dsl {
    namespace word {
        namespace emit {
            struct NetworkEmit {
                NetworkEmit() = default;

                /// The target to send this serialised packet to
                std::string target;
                /// The hash identifying the type of object
                uint64_t hash{0};
                /// The serialised data
                std::vector<uint8_t> payload;
                /// If the message should be sent reliably
                bool reliable{false};
            };

            /**
             * Emits data over the network to other NUClear environments.
             *
             * @code emit<Scope::NETWORK>(data, target, reliable, dataType); @endcode
             * Data emitted under this scope can be sent by name to other NUClear systems or to all NUClear systems
             * connected to the NUClear network.
             * When sent the data is serialized; the associated serialization and deserialization of the object is
             * handled by NUClear.
             *
             * These messages can be sent using either an unreliable protocol that does not guarantee delivery, or
             * using a reliable protocol that does.
             *
             * @attention
             *  Note that if the target system is not connected to the network, the emit will be ignored even if
             *  reliable is enabled.
             *
             * @attention
             *  Data sent under this scope will only trigger reactions (create tasks) for any associated network
             *  requests of this datatype.
             *  For example:
             *  @code on<Network<T>> @endcode
             *  Tasks generated by this emission are assigned to the threadpool on the target environment.
             *
             * @tparam DataType the type of the data to send
             *
             * @param data      the data to emit
             * @param target    The name of the system to send to, or empty for all systems.
             *                  Defaults to all (an empty string).
             * @param reliable  True if the delivery of the message should be guaranteed.
             *                  Defaults to false.
             */
            template <typename DataType>
            struct Network {

                static void emit(PowerPlant& powerplant,
                                 const std::shared_ptr<DataType>& data,
                                 std::string target = "",
                                 bool reliable      = false) {

                    auto e = std::make_unique<NetworkEmit>();

                    e->target   = std::move(target);
                    e->hash     = util::serialise::Serialise<DataType>::hash();
                    e->payload  = util::serialise::Serialise<DataType>::serialise(*data);
                    e->reliable = reliable;

                    powerplant.emit<Inline>(e);
                }

                static void emit(PowerPlant& powerplant, const std::shared_ptr<DataType>& data, bool reliable) {
                    emit(powerplant, data, "", reliable);
                }
            };

        }  // namespace emit
    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_EMIT_NETWORK_HPP
