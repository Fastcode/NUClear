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

#ifndef NUCLEAR_DSL_FUSION_FUSION_HAS_NUCLEAR_DSL_METHOD_HPP
#define NUCLEAR_DSL_FUSION_FUSION_HAS_NUCLEAR_DSL_METHOD_HPP

#include "NoOp.hpp"

/**
 * This macro to create an SFINAE type to check for the existence of a NUClear DSL method in a given class.
 *
 * The macro generates a template struct that can be used to determine if a class contains a specific method.
 *
 * The macro works by attempting to instantiate a function pointer to the method in question.
 * If the method exists, the instantiation will succeed and the struct will inherit from
 * `std::true_type`. Otherwise, it will inherit from `std::false_type`.
 *
 * @note This macro assumes that the method being checked for is a template method that can be instantiated with
 * `ParsedNoOp`.
 */
// If you can find a way to do SFINAE in a constexpr for a variable function name let me know
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define HAS_NUCLEAR_DSL_METHOD(Method)                                                 \
    template <typename Word>                                                           \
    struct has_##Method {                                                              \
    private:                                                                           \
        template <typename R, typename... A>                                           \
        static auto test_func(R (*)(A...)) -> std::true_type;                          \
        static auto test_func(...) -> std::false_type;                                 \
                                                                                       \
        /* Parenthesis would literally break this code*/                               \
        template <typename U> /* NOLINTNEXTLINE(bugprone-macro-parentheses) */         \
        static auto test(int) -> decltype(test_func(&U::template Method<ParsedNoOp>)); \
        template <typename>                                                            \
        static auto test(...) -> std::false_type;                                      \
                                                                                       \
    public:                                                                            \
        static constexpr bool value = decltype(test<Word>(0))::value;                  \
    }

#endif  // NUCLEAR_DSL_FUSION_FUSION_HAS_NUCLEAR_DSL_METHOD_HPP
