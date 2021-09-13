/*
 * Copyright (C) 2013      Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *               2014-2017 Trent Houliston <trent@houliston.me>
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

#ifndef NUCLEAR_UTIL_CALLABLEINFO_HPP
#define NUCLEAR_UTIL_CALLABLEINFO_HPP

namespace NUClear {
namespace util {

    template <typename Ret, typename... Args>
    struct function_info {
        using return_type = Ret;
        using arguments   = std::tuple<Args...>;
    };

    // Given an instance of an object, we have to extract its operator member function
    template <typename T>
    struct CallableInfo : public CallableInfo<decltype(&std::remove_reference_t<T>::operator())> {};

    // Member functions
    // Regular
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...)> : public function_info<Ret, Args...> {};
    // C Variadic
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...)> : public function_info<Ret, Args...> {};
    // Types that have cv-qualifiers
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) volatile> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const volatile> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) volatile> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const volatile> : public function_info<Ret, Args...> {};
// Types that have ref-qualifiers
#if !defined(__GNUC__) || __GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...)&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) volatile&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const volatile&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...)&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) volatile&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const volatile&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) &&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const&&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) volatile&&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args...) const volatile&&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) &&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const&&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) volatile&&> : public function_info<Ret, Args...> {};
    template <typename T, typename Ret, typename... Args>
    struct CallableInfo<Ret (T::*)(Args..., ...) const volatile&&> : public function_info<Ret, Args...> {};
#endif

    // Function Types
    // Regular
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...)> : public function_info<Ret, Args...> {};
    // C Variadic
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...)> : public function_info<Ret, Args...> {};
    // Types that have cv-qualifiers
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) volatile> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const volatile> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) volatile> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const volatile> : public function_info<Ret, Args...> {};
// Types that have ref-qualifiers
#if !defined __GNUC__ || __GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...)&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) volatile&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const volatile&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...)&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) volatile&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const volatile&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) &&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const&&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) volatile&&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args...) const volatile&&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) &&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const&&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) volatile&&> : public function_info<Ret, Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret(Args..., ...) const volatile&&> : public function_info<Ret, Args...> {};
#endif

    // Function Pointers
    // Regular
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*)(Args...)> : public function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*&)(Args...)> : public function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*const)(Args...)> : public function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*const&)(Args...)> : public function_info<Ret(*), Args...> {};
    // C Variadic
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*)(Args..., ...)> : public function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*&)(Args..., ...)> : public function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*const)(Args..., ...)> : public function_info<Ret(*), Args...> {};
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (*const&)(Args..., ...)> : public function_info<Ret(*), Args...> {};

    // Function References
    // Regular
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (&)(Args...)> : public function_info<Ret(&), Args...> {};
    // C Variadic
    template <typename Ret, typename... Args>
    struct CallableInfo<Ret (&)(Args..., ...)> : public function_info<Ret(&), Args...> {};

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_CALLABLEINFO_HPP
