/*
 * MIT License
 *
 * Copyright (c) 2014 NUClear Contributors
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

#ifndef NUCLEAR_DSL_FUSION_HPP
#define NUCLEAR_DSL_FUSION_HPP

#include "fusion/Fuse.hpp"
#include "fusion/hook/Bind.hpp"
#include "fusion/hook/Get.hpp"
#include "fusion/hook/Group.hpp"
#include "fusion/hook/Pool.hpp"
#include "fusion/hook/PostRun.hpp"
#include "fusion/hook/PreRun.hpp"
#include "fusion/hook/Precondition.hpp"
#include "fusion/hook/Priority.hpp"
#include "fusion/hook/RunInline.hpp"
#include "fusion/hook/Scope.hpp"


namespace NUClear {
namespace dsl {

    /// All of the words from a reaction handle "fused" together into one type
    template <typename Word1, typename... WordN>
    struct Fusion {

        using BindFusion         = fusion::Fuse<fusion::hook::Bind, Word1, WordN...>;
        using GetFusion          = fusion::Fuse<fusion::hook::Get, Word1, WordN...>;
        using GroupFusion        = fusion::Fuse<fusion::hook::Group, Word1, WordN...>;
        using PoolFusion         = fusion::Fuse<fusion::hook::Pool, Word1, WordN...>;
        using PostRunFusion      = fusion::Fuse<fusion::hook::PostRun, Word1, WordN...>;
        using PreRunFusion       = fusion::Fuse<fusion::hook::PreRun, Word1, WordN...>;
        using PreconditionFusion = fusion::Fuse<fusion::hook::Precondition, Word1, WordN...>;
        using PriorityFusion     = fusion::Fuse<fusion::hook::Priority, Word1, WordN...>;
        using RunInlineFusion    = fusion::Fuse<fusion::hook::RunInline, Word1, WordN...>;
        using ScopeFusion        = fusion::Fuse<fusion::hook::Scope, Word1, WordN...>;

        template <typename DSL, typename... Args>
        static auto bind(Args&&... args) -> decltype(BindFusion::call(std::forward<Args>(args)...)) {
            return BindFusion::call(std::forward<Args>(args)...);
        }

        template <typename DSL, typename... Args>
        static auto get(Args&&... args) -> decltype(GetFusion::call(std::forward<Args>(args)...)) {
            return GetFusion::call(std::forward<Args>(args)...);
        }

        template <typename DSL, typename... Args>
        static auto group(Args&&... args) -> decltype(GroupFusion::call(std::forward<Args>(args)...)) {
            return GroupFusion::call(std::forward<Args>(args)...);
        }

        template <typename DSL, typename... Args>
        static auto pool(Args&&... args) -> decltype(PoolFusion::call(std::forward<Args>(args)...)) {
            return PoolFusion::call(std::forward<Args>(args)...);
        }

        template <typename DSL, typename... Args>
        static auto post_run(Args&&... args) -> decltype(PostRunFusion::call(std::forward<Args>(args)...)) {
            return PostRunFusion::call(std::forward<Args>(args)...);
        }

        template <typename DSL, typename... Args>
        static auto pre_run(Args&&... args) -> decltype(PreRunFusion::call(std::forward<Args>(args)...)) {
            return PreRunFusion::call(std::forward<Args>(args)...);
        }

        template <typename DSL, typename... Args>
        static auto precondition(Args&&... args) -> decltype(PreconditionFusion::call(std::forward<Args>(args)...)) {
            return PreconditionFusion::call(std::forward<Args>(args)...);
        }

        template <typename DSL, typename... Args>
        static auto priority(Args&&... args) -> decltype(PriorityFusion::call(std::forward<Args>(args)...)) {
            return PriorityFusion::call(std::forward<Args>(args)...);
        }

        template <typename DSL, typename... Args>
        static auto run_inline(Args&&... args) -> decltype(RunInlineFusion::call(std::forward<Args>(args)...)) {
            return RunInlineFusion::call(std::forward<Args>(args)...);
        }

        template <typename DSL, typename... Args>
        static auto scope(Args&&... args) -> decltype(ScopeFusion::call(std::forward<Args>(args)...)) {
            return ScopeFusion::call(std::forward<Args>(args)...);
        }
    };

}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_HPP
