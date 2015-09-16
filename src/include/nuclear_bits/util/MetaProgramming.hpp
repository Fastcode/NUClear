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

#ifndef NUCLEAR_UTIL_METAPROGRAMMING_H
#define NUCLEAR_UTIL_METAPROGRAMMING_H

#include <type_traits>

namespace NUClear {
    namespace util {
        
        /**
         * @namespace NUClear::metaprogramming::Meta
         *
         * @brief This namespace contains many tools that make metaprogramming easier to read and less unwieldy
         *
         * @details
         *  This holds many types that are able to take a lot of the required qualifiers away from the typetrais.
         *  This is done by using using statements to implicity write the qualifiers using a much more readable
         *  syntax.
         *  This can be easily done as all of the metafunctions that exist within typetraits follow a common pattern.
         *
         * @author Trent Houliston
         */
        namespace Meta {
            
            /**
             * @brief Executes a metafunction and becomes the result.
             *
             * @tparam T the metafunction to return the result of.
             *
             * @return The resulting type of the metafunction.
             */
            template <typename T>
            using Do = typename T::type;
            
            /**
             * @brief Given a predacate function, becomes either the if or else (else defaults to void).
             *
             * @tparam Predecate the boolean statement that returns true_type or false_type.
             * @tparam Then      the type to become if the predecate evaluates to true_type.
             * @tparam Else      the type to become if the predecate evaluates to false_type (default void).
             */
            template <typename Predecate, typename Then, typename Else = void>
            using If = Do<std::conditional<Predecate::value, Then, Else>>;
            
            /**
             * @brief Returns the const version of the passed type.
             *
             * @tparam T the type to add the const qualifier to.
             *
             * @return the passed type T with a const qualifier.
             */
            template <typename T>
            using AddConst = Do<std::add_const<T>>;
            
            /**
             * @brief Converts the passed type into an LVale of that type (reference).
             *
             * @tparam T the type to add the LRef qualifier to.
             *
             * @return the passed type T with an LRef qualifier.
             */
            template <typename T>
            using AddLRef = Do<std::add_lvalue_reference<T>>;
            
            /**
             * @brief Removes the reference qualifiers from the passed type.
             *
             * @tparam T the type to remove the reference qualifier to.
             *
             * @return the passed type T without its reference qualifier.
             */
            template <typename T>
            using RemoveRef = Do<std::remove_reference<T>>;
            
            /**
             * @brief Becomes true_type if all of the predicates pass, and false_type if any fail.
             *
             * @tparam T the set of condtions to check.
             *
             * @return true_type if all of the conditions evaluate to true, false_type otherwise.
             */
            template <typename... T>
            struct All : std::true_type {};
            template <typename Head, typename... Tail>
            struct All<Head, Tail...> : If<Head, All<Tail...>, std::false_type> {};
            
            /**
             * @brief Becomes true_type if any of the predicates pass, and false_type they all fail.
             *
             * @tparam T the set of condtions to check.
             *
             * @return true_type if any of the conditions evaluate to true, false_type otherwise.
             */
            template <typename... T>
            struct Any : std::false_type {};
            template <typename Head, typename... Tail>
            struct Any<Head, Tail...> : If<Head, std::true_type, Any<Tail...>> {};
            
            /**
             * @brief Becomes the inverse to the boolean type passed.
             *
             * @tparam T the type to attempt to inverse.
             *
             * @return true_type if T is false_type, false_type if T is true_type.
             */
            template <typename Condition>
            using Not = If<Condition, std::false_type, std::true_type>;
            
            /**
             * @brief This type is valid if all of the conditions succeed, otherwise it is an invalid type.
             *
             * @details
             *  By becoming an invalid type if one of the conditions fail. This allows it to be used as an
             *  EnableIf condition, where a function will only work if all conditions pass
             *
             * @tparam Conditions the condtions to validate
             *
             * @returns A valid type if all condtions pass, an invalid type otherwise
             */
            template <typename Condition, typename Value = void>
            using EnableIf = Do<std::enable_if<Condition::value, Value>>;
            
        }
    }
}

#endif
