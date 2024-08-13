/*
 * MIT License
 *
 * Copyright (c) 2015 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_CALLABLE_INFO_HPP
#define NUCLEAR_UTIL_CALLABLE_INFO_HPP

namespace NUClear {
namespace util {

    template <typename Ret, typename... Args>
    struct function_info {
        using return_type = Ret;
        using arguments   = std::tuple<Args...>;
    };

    // Given an instance of an object, we have to extract its operator member function
    template <typename T>
    struct CallableInfo : CallableInfo<decltype(&std::remove_reference_t<T>::operator())> {};

    // Member functions
    // Regular
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...)> : function_info<Ret, Args...> {};
    // C Variadic
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...)> : function_info<Ret, Args...> {};
    // Types that have cv-qualifiers
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) volatile> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const volatile> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) volatile> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const volatile> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...)&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) volatile&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const volatile&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...)&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) volatile&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const volatile&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) &&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const&&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) volatile&&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const volatile&&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) &&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const&&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) volatile&&> : function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const volatile&&> : function_info<Ret, Args...> {};

    // Function Types
    // Regular
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...)> : function_info<Ret, Args...> {};
    // C Variadic
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...)> : function_info<Ret, Args...> {};
    // Types that have cv-qualifiers
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) volatile> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const volatile> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) volatile> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const volatile> : function_info<Ret, Args...> {};
// Types that have ref-qualifiers
#if !defined __GNUC__ || __GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...)&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) volatile&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const volatile&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...)&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) volatile&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const volatile&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) &&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const&&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) volatile&&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const volatile&&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) &&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const&&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) volatile&&> : function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const volatile&&> : function_info<Ret, Args...> {};
#endif

    // Function Pointers
    // Regular
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*)(Args...)> : function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*&)(Args...)> : function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*const)(Args...)> : function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*const&)(Args...)> : function_info<Ret(*), Args...> {};
    // C Variadic
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*)(Args..., ...)> : function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*&)(Args..., ...)> : function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*const)(Args..., ...)> : function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*const&)(Args..., ...)> : function_info<Ret(*), Args...> {};

    // Function References
    // Regular
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (&)(Args...)> : function_info<Ret(&), Args...> {};
    // C Variadic
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (&)(Args..., ...)> : function_info<Ret(&), Args...> {};

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_CALLABLE_INFO_HPP
