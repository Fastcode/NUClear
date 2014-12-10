/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
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

#ifndef NUCLEAR_DSL_FUSION_H
#define NUCLEAR_DSL_FUSION_H

namespace NUClear {
    namespace dsl {

        // SFINAE for testing the existence of the operational functions in a type
        namespace {
            template<typename T>
            struct has_function {
            private:
                typedef std::true_type yes;
                typedef std::false_type no;

                template<typename U> static auto test_bind(int) -> decltype(U::bind(), yes());
                template<typename> static no test_bind(...);

                template<typename U> static auto test_get(int) -> decltype(U::get(), yes());
                template<typename> static no test_get(...);

                template<typename U> static auto test_precondition(int) -> decltype(U::precondition(), yes());
                template<typename> static no test_precondition(...);

                template<typename U> static auto test_postcondition(int) -> decltype(U::postcondition(), yes());
                template<typename> static no test_postcondition(...);

            public:

                static constexpr bool bind = std::is_same<decltype(test_bind<T>(0)),yes>::value;
                static constexpr bool get = std::is_same<decltype(test_get<T>(0)),yes>::value;
                static constexpr bool precondition = std::is_same<decltype(test_precondition<T>(0)),yes>::value;
                static constexpr bool postcondition = std::is_same<decltype(test_postcondition<T>(0)),yes>::value;
            };

            struct NoOp {
                static void bind() {}
                static std::tuple<> get() { return std::tuple<>(); }
                static bool precondition() { return true; }
                static void postcondition() {}
            };

            template <typename TData>
            struct Tuplify {

                static std::tuple<TData> make(TData&& data) {
                    return data;
                }
            };

            template <typename... TData>
            struct Tuplify<std::tuple<TData...>> {

                static std::tuple<TData...> make(std::tuple<TData...>&& data) {
                    return data;
                }
            };
        }

        template <typename... TWords>
        struct Fusion {

            // Fuse all the binds
            static void bind() {
                auto x = {
                (std::conditional<has_function<TWords>::bind, TWords, NoOp>::type::bind(), 0)...
                };
            }

            // Fuses all the gets if they exist
            static auto get() -> decltype(std::tuple_cat((Tuplify<decltype(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::get())>::make(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::get()))...)) {
                return std::tuple_cat((Tuplify<decltype(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::get())>::make(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::get()))...);
            }

            // Fuses all the postconditions if they exist
            static void postcondition() {
                auto x = {
                (std::conditional<has_function<TWords>::postcondition, TWords, NoOp>::type::postcondition(), 0)...
                };
            }

            // Fuse all the preconditions
            static bool precondition() {

                bool result = true;

                for(bool condition : { (std::conditional<has_function<TWords>::precondition, TWords, NoOp>::type::precondition())... }) {
                    result &= condition;
                }

                return result;

            }
        };
    }
}

#endif
