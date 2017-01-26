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

#ifndef NUCLEAR_UTIL_GENERATE_REACTION_HPP
#define NUCLEAR_UTIL_GENERATE_REACTION_HPP

#include "nuclear_bits/dsl/word/emit/Direct.hpp"
#include "nuclear_bits/threading/Reaction.hpp"
#include "nuclear_bits/util/get_identifier.hpp"

namespace NUClear {
namespace util {

    template <typename DSL, typename BindType, typename Function>
    std::unique_ptr<threading::Reaction> generate_reaction(
        Reactor& reactor,
        const std::string& label,
        Function&& callback,
        std::function<void(threading::Reaction&)> unbind = std::function<void(threading::Reaction&)>()) {

        // Get our identifier string
        std::vector<std::string> identifier =
            util::get_identifier<typename DSL::DSL, Function>(label, reactor.reactor_name);

        // Get our powerplant
        auto& powerplant = reactor.powerplant;

        // Create our unbinder
        auto unbinder = [&powerplant, unbind](threading::Reaction& r) {
            powerplant.emit<dsl::word::emit::Direct>(std::make_unique<dsl::operation::Unbind<BindType>>(r.id));
            if (unbind) {
                unbind(r);
            }
        };

        // Create our reaction
        return std::make_unique<threading::Reaction>(
            reactor, std::move(identifier), std::forward<Function>(callback), std::move(unbinder));
    }

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_GENERATE_REACTION_HPP
