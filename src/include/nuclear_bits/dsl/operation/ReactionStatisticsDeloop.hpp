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

#ifndef NUCLEAR_DSL_OPERATION_REACTIONSTATISTICSDELOOP_HPP
#define NUCLEAR_DSL_OPERATION_REACTIONSTATISTICSDELOOP_HPP

namespace NUClear {
namespace dsl {
    namespace operation {

        /**
         * @brief Prevent recursive looping when using the ReactionStatistics system
         *
         * @details When utilising the ReactionStatistics system there exists an issue in that the reaction that
         *          uses the reaction statistics system will itself generate reaction statistics. This causes an
         *          infinite loop in the system that will eventually cause a stack overflow and crash. To avoid this
         *          this type overloads the method and prevents reactions that use ReactionStatistics from creating
         *          them.
         */
        template <>
        struct CacheGet<message::ReactionStatistics> {

            template <typename DSL>
            static inline std::shared_ptr<const message::ReactionStatistics> get(threading::Reaction& r) {

                // Check if we are triggering ourselves, and if so stop it!
                auto current = threading::ReactionTask::get_current_task();
                if (current && r.id == current->parent.id) {
                    return nullptr;
                }
                else {
                    return store::ThreadStore<std::shared_ptr<message::ReactionStatistics>>::value == nullptr
                               ? store::DataStore<message::ReactionStatistics>::get()
                               : *store::ThreadStore<std::shared_ptr<message::ReactionStatistics>>::value;
                }
            }
        };
    }

}  // namespace message
}  // namespace NUClear

#endif  // NUCLEAR_DSL_OPERATION_REACTIONSTATISTICSDELOOP_HPP
