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

#ifndef NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP
#define NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP

#include "nuclear"
#include "nuclear_bits/extension/network/NUClearNetwork.hpp"

namespace NUClear {
namespace extension {

    class NetworkController : public Reactor {
    public:
        explicit NetworkController(std::unique_ptr<NUClear::Environment> environment);

    private:
        /// Our NUClearNetwork object that handles the networking
        network::NUClearNetwork network;

        /// The reaction that handles timed events from the network
        ReactionHandle process_handle;
        /// The reactions that listen for io
        std::vector<ReactionHandle> listen_handles;

        /// Mutex to guard the list of reactions
        std::mutex reaction_mutex;
        /// Map of type hashes to reactions that are interested in them
        std::multimap<uint64_t, std::shared_ptr<threading::Reaction>> reactions;
    };

}  // namespace extension
}  // namespace NUClear

#endif  // NUCLEAR_EXTENSION_NETWORKCONTROLLER_HPP
