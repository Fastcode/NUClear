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
            
            struct NoOp {
                template <typename DSL, typename TFunc>
                static inline void bind(TFunc&&) {}
                
                template <typename DSL>
                static inline std::tuple<> get() { return std::tuple<>(); }
                
                template <typename DSL>
                static inline bool precondition() { return true; }
                
                template <typename DSL>
                static inline void postcondition() {}
            };
            
            template<typename T>
            struct has_function {
            private:
                typedef std::true_type yes;
                typedef std::false_type no;

                template<typename U> static auto test_bind(int) -> decltype(U::template bind<NoOp>(0), yes());
                template<typename> static no test_bind(...);

                template<typename U> static auto test_get(int) -> decltype(U::template get<NoOp>(), yes());
                template<typename> static no test_get(...);

                template<typename U> static auto test_precondition(int) -> decltype(U::template precondition<NoOp>(), yes());
                template<typename> static no test_precondition(...);

                template<typename U> static auto test_postcondition(int) -> decltype(U::template postcondition<NoOp>
                                                                                     (), yes());
                template<typename> static no test_postcondition(...);

            public:

                static constexpr bool bind = std::is_same<decltype(test_bind<T>(0)),yes>::value;
                static constexpr bool get = std::is_same<decltype(test_get<T>(0)),yes>::value;
                static constexpr bool precondition = std::is_same<decltype(test_precondition<T>(0)),yes>::value;
                static constexpr bool postcondition = std::is_same<decltype(test_postcondition<T>(0)),yes>::value;
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
            template <typename DSL, typename TFunc>
            static void bind(TFunc&& callback) {
                auto x = {
                    (std::conditional<has_function<TWords>::bind, TWords, NoOp>::type::template bind<DSL>(std::forward<TFunc>(callback)), 0)...
                };
            }

            template <typename DSL>
            static auto get() -> decltype(std::tuple_cat((Tuplify<decltype(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::template get<DSL>())>::make(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::template get<DSL>()))...)) {
                
                return std::tuple_cat((Tuplify<decltype(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::template get<DSL>())>::make(std::conditional<has_function<TWords>::get, TWords, NoOp>::type::template get<DSL>()))...);
            }

            template <typename DSL>
            static void postcondition() {
                auto x = {
                (std::conditional<has_function<TWords>::postcondition, TWords, NoOp>::type::template postcondition<DSL>(), 0)...
                };
            }
            
            template <typename DSL>
            static bool precondition() {

                bool result = true;

                for(bool condition : { (std::conditional<has_function<TWords>::precondition, TWords, NoOp>::type::template precondition<DSL>())... }) {
                    result &= condition;
                }

                return result;

            }
        };
    }
}

#endif
