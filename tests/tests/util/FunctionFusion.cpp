/*
 * MIT License
 *
 * Copyright (c) 2022 NUClear Contributors
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

#include "util/FunctionFusion.hpp"

#include <catch2/catch_test_macros.hpp>
#include <tuple>
#include <utility>
#include <vector>


// There are several linting rules from clang-tidy that will trigger in this code.
// However as the point of these tests is to show what the behaviour is in these situations they are suppressed for this
// file
// NOLINTBEGIN(bugprone-use-after-move)

namespace {  // Make everything here internal linkage

/**
 * This struct is used to test what is passed from FunctionFusion to the call function.
 *
 * It has four different versions of append to see which one is called
 * The ones that take rvalue references will move the arguments into new temporaries to ensure the original arguments
 * will be empty after the function runs.
 */
struct Appender {

    /// rvalue/rvalue
    static std::vector<char> append(std::vector<char>&& x, std::vector<char>&& y) {
        const std::vector<char> x1 = std::move(x);
        const std::vector<char> y1 = std::move(y);

        std::vector<char> out = {'r', 'r'};
        out.insert(out.end(), x1.begin(), x1.end());
        out.insert(out.end(), y1.begin(), y1.end());

        return out;
    }

    /// rvalue/lvalue
    static std::vector<char> append(std::vector<char>&& x, const std::vector<char>& y) {
        const std::vector<char> x1 = std::move(x);
        const auto& y1             = y;

        std::vector<char> out = {'l', 'r'};
        out.insert(out.end(), x1.begin(), x1.end());
        out.insert(out.end(), y1.begin(), y1.end());

        return out;
    }

    /// lvalue/rvalue
    static std::vector<char> append(const std::vector<char>& x, std::vector<char>&& y) {
        const auto& x1             = x;
        const std::vector<char> y1 = std::move(y);

        std::vector<char> out = {'l', 'r'};
        out.insert(out.end(), x1.begin(), x1.end());
        out.insert(out.end(), y1.begin(), y1.end());

        return out;
    }

    /// lvalue/lvalue
    static std::vector<char> append(const std::vector<char>& x, const std::vector<char>& y) {
        const auto& x1 = x;
        const auto& y1 = y;

        std::vector<char> out = {'l', 'l'};
        out.insert(out.end(), x.begin(), x.end());
        out.insert(out.end(), y.begin(), y.end());

        return out;
    }
};

template <typename T>
struct AppendCaller {
    template <typename... Args>
    static auto call(Args&&... args) -> decltype(T::append(std::forward<Args>(args)...)) {
        return T::append(std::forward<Args>(args)...);
    }
};

template <int Shared, typename... Args>
auto do_fusion(Args&&... args) {
    return NUClear::util::FunctionFusion<std::tuple<Appender, Appender>,
                                         decltype(std::forward_as_tuple(std::forward<Args>(args)...)),
                                         AppendCaller,
                                         std::tuple<>,
                                         Shared>::call(std::forward<Args>(args)...);
}

}  // namespace

SCENARIO("Shared arguments to FunctionFusion are not moved", "[util][FunctionFusion][shared][move]") {

    WHEN("calling append with 1 shared and 1 selected arguments") {
        std::vector<char> shared({'s'});
        std::vector<char> arg1({'1'});
        std::vector<char> arg2({'2'});

        auto result = do_fusion<1>(std::move(shared), std::move(arg1), std::move(arg2));

        const auto& r1 = std::get<0>(result);
        const auto& r2 = std::get<1>(result);

        THEN("the results are correct") {
            CHECK(r1 == std::vector<char>({'l', 'r', 's', '1'}));
            CHECK(r2 == std::vector<char>({'l', 'r', 's', '2'}));
        }
        THEN("the shared argument is not moved") {
            CHECK(shared == std::vector<char>({'s'}));
        }
        THEN("the selected arguments are moved") {
            CHECK(arg1.empty());
            CHECK(arg2.empty());
        }
    }

    WHEN("calling append with 0 shared and 2 selected arguments") {
        std::vector<char> arg1({'1'});
        std::vector<char> arg2({'2'});
        std::vector<char> arg3({'3'});
        std::vector<char> arg4({'4'});

        auto result = do_fusion<0>(std::move(arg1), std::move(arg2), std::move(arg3), std::move(arg4));

        const auto& r1 = std::get<0>(result);
        const auto& r2 = std::get<1>(result);

        THEN("the results are correct") {
            CHECK(r1 == std::vector<char>({'r', 'r', '1', '2'}));
            CHECK(r2 == std::vector<char>({'r', 'r', '3', '4'}));
        }
        THEN("the selected arguments are moved") {
            CHECK(arg1.empty());
            CHECK(arg2.empty());
            CHECK(arg3.empty());
            CHECK(arg4.empty());
        }
    }
}

// NOLINTEND(bugprone-use-after-move)
