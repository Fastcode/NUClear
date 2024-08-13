/*
 * MIT License
 *
 * Copyright (c) 2013 NUClear Contributors
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

#include "ReactionHandle.hpp"

#include <memory>

#include "Reaction.hpp"

namespace NUClear {
namespace threading {

    ReactionHandle::ReactionHandle(const std::shared_ptr<Reaction>& context) : context(context) {}

    ReactionHandle& ReactionHandle::enable() {
        auto c = context.lock();
        if (c) {
            c->enabled = true;
        }
        return *this;
    }

    ReactionHandle& ReactionHandle::disable() {
        auto c = context.lock();
        if (c) {
            c->enabled = false;
        }
        return *this;
    }

    ReactionHandle& ReactionHandle::enable(const bool& set) {
        auto c = context.lock();
        if (c) {
            c->enabled = set;
        }
        return *this;
    }

    bool ReactionHandle::enabled() const {
        auto c = context.lock();
        return c ? bool(c->enabled) : false;
    }

    // Unbind mutates the reaction which this is a handle for
    // NOLINTNEXTLINE(readability-make-member-function-const)
    void ReactionHandle::unbind() {
        auto c = context.lock();
        if (c) {
            c->unbind();
        }
    }

    ReactionHandle::operator bool() const {
        return bool(context.lock());
    }

}  // namespace threading
}  // namespace NUClear
