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

#ifndef NUCLEAR_DSL_WORD_NETWORK_HPP
#define NUCLEAR_DSL_WORD_NETWORK_HPP

#include "../../threading/Reaction.hpp"
#include "../../util/network/sock_t.hpp"
#include "../../util/serialise/Serialise.hpp"
#include "../store/ThreadStore.hpp"
#include "../trait/is_transient.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        template <typename T>
        struct NetworkData : std::shared_ptr<T> {
            NetworkData() : std::shared_ptr<T>() {}
            explicit NetworkData(T* ptr) : std::shared_ptr<T>(ptr) {}
            NetworkData(const std::shared_ptr<T>& ptr) : std::shared_ptr<T>(ptr) {}
        };

        struct NetworkSource {
            std::string name;
            util::network::sock_t address{};
            bool reliable{false};
        };

        struct NetworkListen {
            uint64_t hash{0};
            std::shared_ptr<threading::Reaction> reaction{nullptr};
        };

        /**
         * NUClear provides a networking protocol to send messages to other devices on the network.
         *
         * @code on<Network<T>>() @endcode
         * This request can be used to communicate with other programs running NUClear.
         * Note that the serialization and deserialization is handled by NUClear.
         *
         * When the reaction is triggered, read-only access to T will be provided to the triggering unit via a callback.
         *
         * @attention
         *  When using an on<Network<T>> request, the associated reaction will only be triggered when T is emitted to
         *  the system using the emission Scope::NETWORK.
         *  Should T be emitted to the system under any other scope, this reaction will not be triggered.
         *
         * @par Implements
         *  Bind, Get
         *
         * @tparam T The datatype on which the reaction callback will be triggered.
         */
        template <typename T>
        struct Network {

            template <typename DSL>
            static void bind(const std::shared_ptr<threading::Reaction>& reaction) {

                auto task = std::make_unique<NetworkListen>();

                task->hash = util::serialise::Serialise<T>::hash();
                reaction->unbinders.push_back([](const threading::Reaction& r) {
                    r.reactor.emit<emit::Inline>(std::make_unique<operation::Unbind<NetworkListen>>(r.id));
                });

                task->reaction = reaction;

                reaction->reactor.emit<emit::Inline>(task);
            }

            template <typename DSL>
            static std::tuple<std::shared_ptr<NetworkSource>, NetworkData<T>> get(threading::ReactionTask& /*task*/) {

                auto* data   = store::ThreadStore<std::vector<uint8_t>>::value;
                auto* source = store::ThreadStore<NetworkSource>::value;

                if (data && source) {

                    // Return our deserialised data
                    return std::make_tuple(std::make_shared<NetworkSource>(*source),
                                           std::make_shared<T>(util::serialise::Serialise<T>::deserialise(*data)));
                }

                // Return invalid data
                return std::make_tuple(std::shared_ptr<NetworkSource>(nullptr), NetworkData<T>(nullptr));
            }
        };

    }  // namespace word

    namespace trait {

        template <typename T>
        struct is_transient<typename word::NetworkData<T>> : std::true_type {};

        template <>
        struct is_transient<typename std::shared_ptr<word::NetworkSource>> : std::true_type {};

    }  // namespace trait
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_NETWORK_HPP
