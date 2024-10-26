/*
 * MIT License
 *
 * Copyright (c) 2024 NUClear Contributors
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
#ifndef NUCLEAR_DSL_WORD_TASK_SCOPE_HPP
#define NUCLEAR_DSL_WORD_TASK_SCOPE_HPP
#include <iostream>

#include "../../id.hpp"
#include "../../threading/ReactionTask.hpp"
#include "../../util/platform.hpp"

namespace NUClear {
namespace dsl {
    namespace word {

        /**
         * This template can be used when you need other DSL words to know that they are running in a specific context.
         *
         * For example, if you want to have a DSL word that modifies how events are emitted within that task you can use
         * this to track when the current task is one with that word.
         * It utilises scope to track when the task is running by storing the task id in a thread local variable.
         * Then when another system needs to know if it is running in that context it can call the in_scope function.
         *
         * @tparam Group a unique type to identify this scope
         */
        template <typename Group>
        struct TaskScope {

            /**
             * Locks the current task id to the word until the lock goes out of scope
             */
            struct Lock {
                explicit Lock(NUClear::id_t old_id) : old_id(old_id) {}

                // No copying the lock
                Lock(const Lock&)            = delete;
                Lock& operator=(const Lock&) = delete;

                // Moving the lock must invalidate the old lock
                Lock(Lock&& other) noexcept : valid(std::exchange(other.valid, false)), old_id(other.old_id) {}
                Lock& operator=(Lock&& other) noexcept {
                    if (this != &other) {
                        valid  = std::exchange(other.valid, false);
                        old_id = other.old_id;
                    }
                }
                ~Lock() {  // Only restore if this lock hasn't been invalidated
                    current_task_id = valid ? old_id : current_task_id;
                }

            private:
                /// If this lock is still valid (hasn't been moved)
                bool valid = true;
                /// The old task id to restore
                NUClear::id_t old_id;
            };

            template <typename DSL>
            static Lock scope(const threading::ReactionTask& task) {
                // Store the old task id
                NUClear::id_t old_id = current_task_id;
                // Set the current task id to the word
                current_task_id = task.id;
                // Return a lock that will restore the old task id
                return Lock(old_id);
            }

            static bool in_scope() {
                // Get the current task id
                const auto* task = threading::ReactionTask::get_current_task();

                // Check if the current task id is the word
                return task != nullptr && task->id == current_task_id;
            }

        private:
            /// The current task id that is running
            // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
            static ATTRIBUTE_TLS NUClear::id_t current_task_id;
        };

        // Initialise the current task id
        template <typename Group>  // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
        ATTRIBUTE_TLS NUClear::id_t TaskScope<Group>::current_task_id{0};

    }  // namespace word
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_WORD_TASK_SCOPE_HPP
