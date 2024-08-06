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
#include "Group.hpp"

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "Pool.hpp"

namespace NUClear {
namespace threading {
    namespace scheduler {

        Group::Group(const util::GroupDescriptor& descriptor) : descriptor(descriptor) {}

        std::shared_ptr<Group::WatcherHandle> Group::add_watcher(const std::function<void()>& fn) {
            if (fn != nullptr) {
                std::lock_guard<std::mutex> lock(mutex);

                auto watcher = std::make_shared<WatcherHandle>(fn);
                watchers.push_back(watcher);
                return watcher;
            }
            return nullptr;
        }

        void Group::notify() {
            // Get the watchers to run out so that we don't hold the lock while running them
            std::vector<std::weak_ptr<WatcherHandle>> to_run;
            /*mutex scope*/ {
                std::lock_guard<std::mutex> lock(mutex);
                to_run.swap(watchers);
            }

            // Run each of the watchers if they are still valid
            for (auto& watcher : to_run) {
                auto w = watcher.lock();
                if (w != nullptr) {
                    w->called = true;
                    w->fn();
                }
            }
        }

    }  // namespace scheduler
}  // namespace threading
}  // namespace NUClear
