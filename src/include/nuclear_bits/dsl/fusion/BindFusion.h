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
#include "nuclear_bits/util/tuplify.h"
#include "nuclear_bits/threading/ReactionHandle.h"
#include "nuclear_bits/dsl/operation/DSLProxy.h"
#include "nuclear_bits/dsl/fusion/has_bind.h"

namespace NUClear {
    namespace dsl {
        namespace fusion {
            
            template <typename Predecate, typename Then, typename Else>
            using If = util::Meta::If<Predecate, Then, Else>;
            
            template <typename Condition, typename Value>
            using EnableIf = util::Meta::EnableIf<Condition, Value>;
            
            template <typename Condition>
            using Not = util::Meta::Not<Condition>;
            
            template <typename... Conditions>
            using All = util::Meta::All<Conditions...>;
            
            template <typename... Conditions>
            using Any = util::Meta::Any<Conditions...>;
            
            // This function is needed because G++ is a little slow when it comes to templated functions and needs some help
            template <typename R, typename... A>
            auto resolve_function_type(R(*)(A...))
            -> R(*)(A...);
            
            template <typename TFuncSignature, typename TFirst, typename... TWords>
            struct BindFission;
            
            template <typename... TWords>
            struct BindFusion;
            
            template <typename TRet, typename TFunc, typename... TRelevant, typename TFirst, typename... TWords>
            struct BindFission<TRet(*)(Reactor&, const std::string&, TFunc, TRelevant...), TFirst, TWords...> {
                
                template <typename DSL, typename... TRemainder>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc&& callback, TRelevant&&... relevant, TRemainder&&... remainder)
                -> decltype(std::tuple_cat(util::tuplify(TFirst::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TRelevant>(relevant)...))
                                           , BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TRemainder>(remainder)...))) {
                    
                    return std::tuple_cat(util::tuplify(TFirst::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TRelevant>(relevant)...))
                                          , BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TRemainder>(remainder)...));
                    
                }
            };
            
            template <typename TFirst, typename... TWords>
            struct BindFusion<TFirst, TWords...> {
            private:
                
                /// Returns either the real type or the proxy if the real type does not have a bind function
                template <typename U>
                using Bind = If<has_bind<U>, U, operation::DSLProxy<U>>;
                
                /// Gets the signature of the bind function (g++ needs some help with this)
                template <typename U, typename DSL, typename TFunc>
                using BindSignature = decltype(resolve_function_type(Bind<U>::template bind<DSL, TFunc>));
                
                /// Builds our fission type (since it's somewhat long we extract it here)
                template <typename U, typename DSL, typename TFunc>
                using Fission = BindFission<BindSignature<U, DSL, TFunc>, Bind<U>, TWords...>;
                
                /// Checks if U has a bind function, and at least one of the following words do
                template <typename U>
                using UsAndChildren = All<has_bind<Bind<U>>, Any<has_bind<Bind<TWords>>...>>;
                
                /// Checks if U has a bind function, and none of the following words do
                template <typename U>
                using UsNotChildren = All<has_bind<Bind<U>>, Not<Any<has_bind<Bind<TWords>>...>>>;
                
                /// Checks if we do not have a bind function, but at least one of the following words do
                template <typename U>
                using NotUsChildren = All<Not<has_bind<Bind<U>>>, Any<has_bind<Bind<TWords>>...>>;
                
            public:
                
                /**
                 * @brief This function is enabled in the situation that TFirst has a bind function, and there
                 *          is a bind function within TWords also.
                 *
                 * @tparam DSL      The parsed DSL that resulted from all these functions
                 * @tparam U        A copy of TFirst used for SFINAE
                 * @tparam TFunc    The type of the callback function passed to bind
                 * @tparam TArgs    The types of the runtime arguments passed into the bind
                 */
                template <typename DSL, typename U = TFirst, typename TFunc, typename... TArgs>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc&& callback, TArgs&&... args)
                -> EnableIf<UsAndChildren<U>,
                decltype(If<UsAndChildren<U>, Fission<U, DSL, TFunc>, NoOp>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...))> {
                    
                    // Fission off the arguments that we need (Fission will rejoin fusion when it's done)
                    return Fission<U, DSL, TFunc>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...);
                }
                
                /**
                 * @brief This function is enabled in the situation that TFirst has a bind function, however there
                 *          are no further bind functions within our words
                 *
                 * @tparam DSL      The parsed DSL that resulted from all these functions
                 * @tparam U        A copy of TFirst used for SFINAE
                 * @tparam TFunc    The type of the callback function passed to bind
                 * @tparam TArgs    The types of the runtime arguments passed into the bind
                 */
                template <typename DSL, typename U = TFirst, typename TFunc, typename... TArgs>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc&& callback, TArgs&&... args)
                -> EnableIf<UsNotChildren<U>, decltype(util::tuplify(Bind<U>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...)))> {
                    
                    // Execute our function
                    return util::tuplify(Bind<U>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...));
                }
                
                /**
                 * @brief This function is enabled in the situation that TFirst does not have a bind function, but there
                 *          are words further down that do
                 *
                 * @tparam DSL      The parsed DSL that resulted from all these functions
                 * @tparam U        A copy of TFirst used for SFINAE
                 * @tparam TFunc    The type of the callback function passed to bind
                 * @tparam TArgs    The types of the runtime arguments passed into the bind
                 */
                template <typename DSL, typename U = TFirst, typename TFunc, typename... TArgs>
                static inline auto bind(Reactor& reactor, const std::string& identifier, TFunc&& callback, TArgs&&... args)
                -> EnableIf<NotUsChildren<U>, decltype(If<NotUsChildren<U>, BindFusion<TWords...>, NoOp>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...))> {
                    
                    // Forward onto the next stage of bind
                    return BindFusion<TWords...>::template bind<DSL>(reactor, identifier, std::forward<TFunc>(callback), std::forward<TArgs>(args)...);
                }
            };
        }
    }
}

#endif
