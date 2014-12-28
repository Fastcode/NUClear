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

#ifndef NUCLEAR_DSL_FUSION_BINDFUSION_H
#define NUCLEAR_DSL_FUSION_BINDFUSION_H

#include "nuclear_bits/util/MetaProgramming.h"
#include "nuclear_bits/threading/ReactionHandle.h"

namespace NUClear {
    namespace dsl {
        namespace fusion {
            
            template <typename Condition, typename Value>
            using EnableIf = util::Meta::EnableIf<Condition, Value>;
            
            template <typename Condition>
            using Not = util::Meta::Not<Condition>;
            
            template <typename... Conditions>
            using All = util::Meta::All<Conditions...>;
            
            template <typename... Conditions>
            using Any = util::Meta::Any<Conditions...>;
            
            
            // SFINAE for testing the existence of the operational functions in a type
            namespace {
                
                /**
                 * @brief SFINAE check to determine if the bind function exists on the passed type
                 *
                 * @tparam T the type to check if the bind funciton exists on (with any paramters)
                 */
                template<typename T>
                struct bind_exists {
                private:
                    typedef std::true_type yes;
                    typedef std::false_type no;
                    
                    template<typename U> static auto test(int) -> decltype(U::template bind<void>, yes());
                    template<typename> static no test(...);
                    
                public:
                    static constexpr bool value = std::is_same<decltype(test<T>(0)),yes>::value;
                };
                
                /**
                 * @brief Used to test that the bind object that was found by bind_exists is in fact a function
                 *
                 * @tparam T       the type to check if bind exists and is a function
                 * @tparam HasBind Boolean value if bind exists in the first place (can't check if it doesn't exist)
                 */
                template <typename T, bool HasBind = bind_exists<T>::value>
                struct has_bind;
                
                /**
                 * @brief When bind does exist we must confirm that it is a function
                 *
                 * @tparam T   the type to check if bind exists and is a function
                 */
                template <typename T>
                struct has_bind<T, true> {
                    static constexpr bool value = std::is_function<decltype(T::template bind<void>)>::value;
                };
                
                /**
                 * @brief If bind does not exist the value will always be false
                 *
                 * @tparam T   the type to check if bind exists and is a function
                 */
                template <typename T>
                struct has_bind<T, false> {
                    static constexpr bool value = false;
                };
            }
            
            template <typename TFuncSignature, typename TFirst, typename... TWords>
            struct BindFission;
            
            template <typename TFirst, typename... TWords>
            struct BindFusion;
            
            template <typename TRet, typename... TRelevant, typename TFirst, typename... TWords>
            struct BindFission<TRet (TRelevant...), TFirst, TWords...> {
                
                template <typename DSL, typename TFunc, typename... TRemainder>
                static inline std::vector<threading::ReactionHandle> bind(Reactor& reactor, const std::string& identifier, TFunc callback, TRelevant... relevant, TRemainder... remainder) {
                    
                    // Call our bind function with the relevant arguments
                    TFirst::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TRelevant>(relevant)...);
                    
                    // Call the remainder of our fusion
                    BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TRemainder>(remainder)...);
                }
            };
            
            template <typename TFirst, typename... TWords>
            struct BindFusion {
                
                
                // Has + futures
                // Has - futures
                // NoHas + futures
                
                template <typename DSL, typename U = TFirst, typename TFunc, typename... TArgs>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc callback, TArgs... args)
                -> EnableIf<All<has_bind<U>, Any<has_bind<TWords>...>>
                , std::vector<threading::ReactionHandle>> {
                    
                    // Us and our children
                    
                    // Fission off the arguments that we need (Fission will rejoin fusion when it's done)
                    return BindFission<decltype(TFirst::template get<DSL>), TFirst, TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...);
                }
                
                template <typename DSL, typename U = TFirst, typename TFunc, typename... TArgs>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc callback, TArgs... args)
                -> EnableIf<All<has_bind<U>, Not<Any<has_bind<TWords>...>>>
                , std::vector<threading::ReactionHandle>> {
                    
                    // Us but not our children
                    
                    // Execute our function
                    TFirst::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...);
                }
                
                template <typename DSL, typename U = TFirst, typename TFunc, typename... TArgs>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc callback, TArgs... args)
                -> EnableIf<All<Not<has_bind<U>>, Any<has_bind<TWords>...>>
                , std::vector<threading::ReactionHandle>> {
                    
                    // Not us but our children
                    // Forward onto the next stage of bind
                    return BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...);
                }
                
                
//                /**
//                 * @brief This function is enabled in the situation that TFirst has a bind function, and there
//                 *          is a bind function within TWords also.
//                 *
//                 * @tparam U        A copy of TFirst used for SFINAE
//                 * @tparam TFunc    The type of the callback function passed to bind
//                 * @tparam TArgs    The types of the runtime arguments passed into the bind
//                 */
//                template <typename DSL, typename U = TFirst, typename TFunc, typename... TArgs>
//                static inline void bind(Reactor& reactor, const std::string& identifier, TFunc callback, TArgs... args) {
//                    
//                }
//                
//                /**
//                 * @brief This function is enabled in the situation that TFirst has a bind function, however there
//                 *          are no further bind functions within our words
//                 *
//                 * @tparam U        A copy of TFirst used for SFINAE
//                 * @tparam TFunc    The type of the callback function passed to bind
//                 * @tparam TArgs    The types of the runtime arguments passed into the bind
//                 */
//                template <typename DSL, typename U = TFirst, typename TFunc, typename... TArgs>
//                static inline void bind(Reactor& reactor, const std::string& identifier, TFunc callback, TArgs... args) {
//
//                }
//                
//                /**
//                 * @brief This function is enabled in the situation that TFirst does not have a bind function, but there
//                 *          are words further down that do
//                 *
//                 * @tparam U        A copy of TFirst used for SFINAE
//                 * @tparam TFunc    The type of the callback function passed to bind
//                 * @tparam TArgs    The types of the runtime arguments passed into the bind
//                 */
//                template <typename DSL, typename U = TFirst, typename TFunc, typename... TArgs>
//                static inline void bind(Reactor& reactor, const std::string& identifier, TFunc callback, TArgs... args) {
//                    
//                    // Forward onto the next stage of bind
//                    BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...);
//                }
            };
        }
    }
}

#endif
