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

#ifndef NUCLEAR_UTIL_FUNCTIONFUSION_HPP
#define NUCLEAR_UTIL_FUNCTIONFUSION_HPP

#include "Sequence.hpp"
#include "tuplify.hpp"

namespace NUClear {
namespace util {

    /**
     * @brief Applies a single set of function fusion with expanded arguments
     * @details Calls the function held in the template type Function.
     *          for the arguments it uses the paramter packs Shared and Selected
     *          to expand the passed tuple args and forward those selected
     *          arguments to the function. This function is normally called by
     *          the other overload of apply_function_fusion_call to get the expanded
     *          paramter packs.
     *
     * @param  args     the arguments that were passed to the superfunction
     *
     * @tparam Function     the struct that holds the call function wrapper to be called
     * @tparam Shared       the index list of shared arguments at the start of the argument pack to use
     * @tparam Selected     the index list of selected arguments in the argument pack to use
     * @tparam Arguments    the types of the arguments passed into the function
     *
     * @return the object returned by the called subfunction
     */
    template <typename Function, int... Shared, int... Selected, typename... Arguments>
    auto apply_function_fusion_call(std::tuple<Arguments...>&& args,
                                    const Sequence<Shared...>&,
                                    const Sequence<Selected...>&)
        -> decltype(Function::call(std::get<Shared>(args)..., std::get<Selected>(args)...)) {
        return Function::call(std::get<Shared>(args)..., std::get<Selected>(args)...);
    }

    /**
     * @brief Applies a single set of function fusion with argument ranges
     * @details Calls the function held in the template type Function.
     *          for the arguments it uses the paramter packs Shared and Selected
     *          to expand the passed tuple args and forward those selected
     *          arguments to the function.
     *
     * @param  args     the arguments that were passed to the superfunction
     *
     * @tparam Function     the struct that holds the call function wrapper to be called
     * @tparam Shared       the number of paramters (from 0) to use in the call
     * @tparam Start        the index of the first argument to pass to the function
     * @tparam End          the index of the element after the last argument to pass to the function
     * @tparam Arguments    the types of the arguments passed into the function
     *
     * @return the object returned by the called subfunction
     */
    template <typename Function, int Shared, int Start, int End, typename... Arguments>
    auto apply_function_fusion_call(std::tuple<Arguments...>&& args)
        -> decltype(apply_function_fusion_call<Function>(std::forward<std::tuple<Arguments...>>(args),
                                                         GenerateSequence<0, Shared>(),
                                                         GenerateSequence<Start, End>())) {
        return apply_function_fusion_call<Function>(std::forward<std::tuple<Arguments...>>(args),
                                                    GenerateSequence<0, Shared>(),
                                                    GenerateSequence<Start, End>());
    }

    template <typename Functions, int Shared, typename Ranges, typename Arguments>
    struct FunctionFusionCaller;

    /**
     * @brief Used to call the result of the function fusion with the given arguments
     * @details Provides a call function that will split the given arguments amoungst the functions
     *          according to the provided ranges.
     *
     * @tparam Functions The functions that we are going to call
     * @tparam Shared    the number of arguments (from 0) to use in all of the calls
     * @tparam Ranges    a set of pairs of integers that describe the first and last
     *                   argument provided to each respective function
     * @tparam Arguments the type of the provided arguments
     */
    template <typename... Functions, int Shared, typename... Ranges, typename... Arguments>
    struct FunctionFusionCaller<std::tuple<Functions...>, Shared, std::tuple<Ranges...>, std::tuple<Arguments...>>
        : public std::true_type {
    private:
        /**
         * @brief Calls a single function in the function set.
         *
         * @param e     the range sequence providing the start and end of the relevant arguments
         * @param args  the arguments to be used to call
         *
         * @tparam Function  the function that we are going to call
         * @tparam Start     the index of the first argument to pass to the function
         * @tparam End       the index of the element after the last argument to pass to the function
         *
         * @return the result of calling this specific function
         */
        template <typename Function, int Start, int End>
        static inline auto call_one(const Sequence<Start, End>&, Arguments&&... args)
            -> decltype(apply_function_fusion_call<Function, Shared, Start, End>(std::forward_as_tuple(args...))) {

            return apply_function_fusion_call<Function, Shared, Start, End>(std::forward_as_tuple(args...));
        }

        /**
         * @brief This function exists unimplemented to absorb incorrect template instansiations.
         *
         * @param  swallows arguments
         *
         * @tparam typename swallows the template paramter
         *
         * @return ignore
         */
        template <typename>
        static inline bool call_one(...);

    public:
        /**
         * @brief Calls the collection of functions with the given arguments.
         *
         * @param args The arguments to be used to call
         *
         * @return A tuple of the returned values, or if the return value was a tuple fuse it
         */
        static inline auto call(Arguments&&... args)
            -> decltype(std::tuple_cat(tuplify(call_one<Functions>(Ranges(), std::forward<Arguments>(args)...))...)) {

            // Now to call all of the sets with their arguments
            return std::tuple_cat(tuplify(call_one<Functions>(Ranges(), std::forward<Arguments>(args)...))...);
        }
    };

    /**
     * @brief SFINAE test struct to see if a function is callable with the provided arguments.
     *
     * @tparam Function  the function to be tested
     * @tparam Shared    the number of paramters (from 0) to use in the call
     * @tparam Start     the index of the first argument to pass to the function
     * @tparam End       the index of the element after the last argument to pass to the function
     * @tparam Arguments the types of the arguments passed into the function
     */
    template <typename Function, int Shared, int Start, int End, typename Arguments>
    struct is_callable {
    private:
        typedef std::true_type yes;
        typedef std::false_type no;

        template <typename F>
        static auto test(int)
            -> decltype(apply_function_fusion_call<F, Shared, Start, End>(std::declval<Arguments>()), yes());
        template <typename>
        static no test(...);

    public:
        static constexpr bool value = std::is_same<decltype(test<Function>(0)), yes>::value;
    };

    template <typename Functions,
              typename Arguments,
              template <typename, typename...>
              class FunctionWrapper,
              typename WrapperArgs,
              int Shared                  = 0,
              int Start                   = Shared,
              int End                     = std::tuple_size<Arguments>::value,
              typename ProcessedFunctions = std::tuple<>,
              typename Ranges             = std::tuple<>>
    struct FunctionFusion;

    /**
     * @brief Splits arguments amongst a series of functions.
     * @details This is the main loop for the function fusion metafunction.
     *          It will do a greedy matching of arguments with function overloads
     *          to try and make all functions callable with the given arguments.
     *          It will share the first arguments (up to but not including Shared)
     *          across all functions.
     *
     *          These functions are provided with a wrapper FunctionWrapper which is
     *          a template that takes their type and uses its ::call function to call
     *          the original functions function.
     *          This allows fusion of functions without knowing the name of the function
     *          that is being fused.
     *
     * @tparam Functions            the functions we are going to call
     * @tparam Arguments            the arguments we are calling the function with
     * @tparam FunctionWrapper      the template that is used to wrap the Function objects
     *                              to be called
     * @tparam WrapperArgs          template types to be used on the FunctionWrapper in addition to the Fuctions type.
     *                              May be empty.
     * @tparam Shared               the number of paramters (from 0) to use in all of the calls
     * @tparam Start                the current attempted index of the first argument to pass to the function
     * @tparam End                  the current attempted index of the element after the last argument to pass to the
     *                              function
     * @tparam ProcessedFunctions   the list of functions that have had found argument sets
     * @tparam ArgumentRanges       the respective argument ranges for the processed functions
     */
    template <typename CurrentFunction,
              typename... Functions,
              typename... Arguments,
              template <typename, typename...>
              class FunctionWrapper,
              typename... WrapperArgs,
              int Shared,
              int Start,
              int End,
              typename... ProcessedFunctions,
              typename... ArgumentRanges>
    struct FunctionFusion<std::tuple<CurrentFunction, Functions...>,
                          std::tuple<Arguments...>,
                          FunctionWrapper,
                          std::tuple<WrapperArgs...>,
                          Shared,
                          Start,
                          End,
                          std::tuple<ProcessedFunctions...>,
                          std::tuple<ArgumentRanges...>>
        : public
          // Test if we have moved into an invalid range
          std::conditional_t<
              (Start > End),
              // We are in an invalid range which makes this path invalid
              /*T*/ std::false_type,
              // Test if we are callable with the current arguments
              /*F*/
              std::conditional_t<
                  is_callable<FunctionWrapper<CurrentFunction, WrapperArgs...>,
                              Shared,
                              Start,
                              End,
                              std::tuple<Arguments...>>::value,
                  // test if our remainder is valid
                  /*T*/
                  std::conditional_t<FunctionFusion<std::tuple<Functions...>,
                                                    std::tuple<Arguments...>,
                                                    FunctionWrapper,
                                                    std::tuple<WrapperArgs...>,
                                                    Shared,
                                                    End,
                                                    sizeof...(Arguments),
                                                    std::tuple<ProcessedFunctions..., CurrentFunction>,
                                                    std::tuple<ArgumentRanges..., Sequence<Start, End>>>::value,
                                     // It is valid, extend from the next step
                                     /*T*/
                                     FunctionFusion<std::tuple<Functions...>,
                                                    std::tuple<Arguments...>,
                                                    FunctionWrapper,
                                                    std::tuple<WrapperArgs...>,
                                                    Shared,
                                                    End,
                                                    sizeof...(Arguments),
                                                    std::tuple<ProcessedFunctions..., CurrentFunction>,
                                                    std::tuple<ArgumentRanges..., Sequence<Start, End>>>,
                                     // It is not valid, try with one less argument assigned
                                     // to us
                                     /*F*/
                                     FunctionFusion<std::tuple<CurrentFunction, Functions...>,
                                                    std::tuple<Arguments...>,
                                                    FunctionWrapper,
                                                    std::tuple<WrapperArgs...>,
                                                    Shared,
                                                    Start,
                                                    End - 1,
                                                    std::tuple<ProcessedFunctions...>,
                                                    std::tuple<ArgumentRanges...>>>,
                  // We are not callable with this number of arguments, try one less
                  /*F*/
                  FunctionFusion<std::tuple<CurrentFunction, Functions...>,
                                 std::tuple<Arguments...>,
                                 FunctionWrapper,
                                 std::tuple<WrapperArgs...>,
                                 Shared,
                                 Start,
                                 End - 1,
                                 std::tuple<ProcessedFunctions...>,
                                 std::tuple<ArgumentRanges...>>>> {};

    /**
     * @brief The termination case for the FunctionFusion metafunction.
     *
     * @details This is the termination case for the FunctionFusion metafunction.
     *          if it reaches this point with a successful combination it will
     *          extend from the FunctionFusionCaller, otherwise it will be a
     *          false_type to indicate its failure.
     */
    template <typename... Arguments,
              template <typename, typename...>
              class FunctionWrapper,
              typename... WrapperArgs,
              int Shared,
              int Start,
              int End,
              typename... ProcessedFunctions,
              typename... Ranges>
    struct FunctionFusion<std::tuple<>,
                          std::tuple<Arguments...>,
                          FunctionWrapper,
                          std::tuple<WrapperArgs...>,
                          Shared,
                          Start,
                          End,
                          std::tuple<ProcessedFunctions...>,
                          std::tuple<Ranges...>>
        : public
          // Check if we used up all of our arguments (and not more than all of our arguments)
          std::conditional_t<(Start == End && Start == int(sizeof...(Arguments))),
                             // We have used up the exact right number of arguments (and everything by this point should
                             // have been callable)
                             /*T*/
                             FunctionFusionCaller<std::tuple<FunctionWrapper<ProcessedFunctions, WrapperArgs...>...>,
                                                  Shared,
                                                  std::tuple<Ranges...>,
                                                  std::tuple<Arguments...>>,
                             // We have the wrong number of arguments left over, this branch is bad
                             /*F*/ std::false_type> {};

}  // namespace util
}  // namespace NUClear

#endif  // NUCLEAR_UTIL_FUNCTIONFUSION_HPP
