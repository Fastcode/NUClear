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

#ifndef NUCLEAR_THREADING_REACTIONHANDLE_HPP
#define NUCLEAR_THREADING_REACTIONHANDLE_HPP

#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <typeindex>
#include <vector>

#include "Reaction.hpp"

namespace NUClear {
namespace threading {

    /**
     * @brief Gives user code access to control the reaction object.
     *
     * @details
     *  This object is given to user code when they create a reaction. It contains functions which allow changing of
     *  the reaction after it has been created such as enabling and disabling its execution.
     */
    class ReactionHandle {
    public:
        /// @brief the reaction that we are managing
        std::weak_ptr<Reaction> context;

        /**
         * @brief
         *  Creates a new ReactionHandle for the reaction that is passed in.
         *
         * @param
         *  context the reaction that we are interacting with.
         */
        explicit ReactionHandle(const std::shared_ptr<Reaction>& context = nullptr) : context(context) {}

        /**
         * @brief
         *  Enables the reaction so that associated tasks will be scheduled and queued when the reaction is triggered.
         */
        inline ReactionHandle& enable() {
            auto c = context.lock();
            if (c) {
                c->enabled = true;
            }
            return *this;
        }

        /**
         * @brief
         *  Disables the reaction.
         *  When disabled, any associated tasks will not be created if triggered.  All reaction configuration is still
         *  available, so that the reaction can be enabled when required.
         *  Note that a reaction which has been bound by an on<Always> request should not be disabled.
         */
        inline ReactionHandle& disable() {
            auto c = context.lock();
            if (c) {
                c->enabled = false;
            }
            return *this;
        }

        /**
         * @brief
         *  Sets the run status of the reaction handle.
         * @param set
         *  true for enable, false for disable
         */
        inline ReactionHandle& enable(const bool& set) {
            auto c = context.lock();
            if (c) {
                c->enabled = set;
            }
            return *this;
        }

        /**
         * @brief
         *  Informs if the reaction is currently enabled.
         *
         * @return
         *  true if enabled, false if disabled
         */
        inline bool enabled() const {
            auto c = context.lock();
            return c ? bool(c->enabled) : false;
        }

        /**
         * @brief
         *  Removes a reaction request from the runtime environment.  This action is not reversible, once a reaction has
         *  been unbound, it is no longer available for further use during the instance of runtime.
         *  This is most commonly used for the unbinding of network configuration before attempting to re-set
         *  configuration details during runtime.
         */
        // NOLINTNEXTLINE(readability-make-member-function-const) unbinding modifies the reaction
        inline void unbind() {
            auto c = context.lock();
            if (c) {
                c->unbind();
            }
        }

        /**
         * @brief
         *  Returns if this reaction handle holds a valid pointer (may be already unbound)
         *
         * @return
         *  true if the reaction held in this is not a nullptr
         */
        inline operator bool() const {
            return bool(context.lock());
        }
    };

}  // namespace threading
}  // namespace NUClear

#endif  // NUCLEAR_THREADING_REACTIONHANDLE_HPP
