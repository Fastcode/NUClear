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
#include "nuclear_bits/dsl/fusion/has_bind.h"

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
            
            
            template <typename TFuncSignature, typename TFirst, typename... TWords>
            struct BindFission;
            
            template <typename TFirst, typename... TWords>
            struct BindFusion;
            
            template <typename TRet, typename... TRelevant, typename TFirst, typename... TWords>
            struct BindFission<TRet (TRelevant...), TFirst, TWords...> {
                
                template <typename DSL, typename TFunc, typename... TRemainder>
                static inline std::vector<threading::ReactionHandle> bind(Reactor& reactor, const std::string& identifier, TFunc callback, TRelevant... relevant, TRemainder... remainder) {
                    
                    // Call our bind function with the relevant arguments
                    std::vector<threading::ReactionHandle> init = TFirst::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TRelevant>(relevant)...);
                    
                    // Call the remainder of our fusion
                    auto newHandles = BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TRemainder>(remainder)...);
                    
                    init.insert(std::end(init), std::begin(newHandles), std::end(newHandles));
                    return init;
                }
            };
            
            template <typename TFirst, typename... TWords>
            struct BindFusion {
                
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
                    return TFirst::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...);
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
