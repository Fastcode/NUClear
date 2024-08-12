/*
 * MIT License
 *
 * Copyright (c) 2018 NUClear Contributors
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

#ifndef NUCLEAR_UTIL_FUNCTIONFUSION_HPP
#define NUCLEAR_UTIL_FUNCTIONFUSION_HPP

#include "Sequence.hpp"
#include "tuplify.hpp"

namespace NUClear {
namespace util {

    /**
     * Applies a single set of function fusion with expanded arguments.
     *
     * Calls the function held in the template type Function.
     * For the arguments it uses the parameter packs Shared and Selected to expand the passed tuple args and forward
     * those selected arguments to the function.
     * This function is normally called by the other overload of apply_function_fusion_call to get the expanded
     * parameter packs.
     *
     * @tparam Function  The struct that holds the call function wrapper to be called
     * @tparam Shared    The index list of shared arguments at the start of the argument pack to use
     * @tparam Selected  The index list of selected arguments in the argument pack to use
     * @tparam Arguments The types of the arguments passed into the function
     *
     * @param  args The arguments that were passed to the superfunction
     *
     * @return The object returned by the called subfunction
     */
    template <typename Function, int... Shared, int... Selected, typename... Arguments>
    auto apply_function_fusion_call(const std::tuple<Arguments...>& args,
                                    const Sequence<Shared...>& /*shared*/,
                                    const Sequence<Selected...>& /*selected*/)
        -> decltype(Function::call(std::get<Shared>(args)..., std::get<Selected>(args)...)) {
        return Function::call(std::get<Shared>(args)..., std::get<Selected>(args)...);
    }

    /**
     * Applies a single set of function fusion with argument ranges
     *
     * Calls the function held in the template type Function.
     * For the arguments it uses the parameter packs Shared and Selected to expand the passed tuple args and forward
     * those selected arguments to the function.
     *
     * @tparam Function   The struct that holds the call function wrapper to be called
     * @tparam Shared     The number of parameters (from 0) to use in the call
     * @tparam Start      The index of the first argument to pass to the function
     * @tparam End        The index of the element after the last argument to pass to the function
     * @tparam Arguments  The types of the arguments passed into the function
     *
     * @param  args     the arguments that were passed to the super-function
     *
     * @return the object returned by the called subfunction
     */
    template <typename Function, int Shared, int Start, int End, typename... Arguments>
    auto apply_function_fusion_call(std::tuple<Arguments...>&& args)
        -> decltype(apply_function_fusion_call<Function>(std::move(args),
                                                         GenerateSequence<0, Shared>(),
                                                         GenerateSequence<Start, End>())) {
        return apply_function_fusion_call<Function>(std::move(args),
                                                    GenerateSequence<0, Shared>(),
                                                    GenerateSequence<Start, End>());
    }

    template <typename Functions, int Shared, typename Ranges, typename Arguments>
    struct FunctionFusionCaller;

    /**
     * Termination case for calling a function fusion.
     *
     * Terminates by just returning an empty tuple.
     *
     * @tparam Shared    The number of arguments (from 0) to use in all of the calls
     * @tparam Arguments The type of the provided arguments
     */
    template <int Shared, typename... Arguments>
    struct FunctionFusionCaller<std::tuple<>, Shared, std::tuple<>, std::tuple<Arguments...>> {
        // This function is just here to satisfy the templates
        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        static std::tuple<> call(Arguments&&... /*args*/) {
            return {};
        }
    };

    /**
     * Used to call the result of the function fusion with the given arguments.
     *
     * Provides a call function that will split the given arguments amongst the functions according to the provided
     * ranges.
     *
     * @tparam CurrentFunction  The current function we are calling in this class
     * @tparam Functions        The remaining functions we are going to call
     * @tparam Shared           The number of arguments (from 0) to use in all of the calls
     * @tparam CurrentRange     The range of arguments to use in the current call
     * @tparam Ranges           Pairs of integers that describe the first and last argument provided to each function
     * @tparam Arguments        The type of the provided arguments
     */
    template <typename CurrentFunction,
              typename... Functions,
              int Shared,
              typename CurrentRange,
              typename... Ranges,
              typename... Arguments>
    struct FunctionFusionCaller<std::tuple<CurrentFunction, Functions...>,
                                Shared,
                                std::tuple<CurrentRange, Ranges...>,
                                std::tuple<Arguments...>> : std::true_type {
    private:
        /**
         * Calls a single function in the function set.
         *
         * @tparam Function  The function that we are going to call
         * @tparam Start     The index of the first argument to pass to the function
         * @tparam End       The index of the element after the last argument to pass to the function
         *
         * @param e     The range sequence providing the start and end of the relevant arguments
         * @param args  The arguments to be used to call
         *
         * @return the result of calling this specific function
         */
        template <typename Function, int Start, int End>
        // It is forwarded as a tuple
        // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
        static auto call_one(const Sequence<Start, End>& /*e*/, Arguments&&... args)
            -> decltype(apply_function_fusion_call<Function, Shared, Start, End>(std::forward_as_tuple(args...))) {

            return apply_function_fusion_call<Function, Shared, Start, End>(std::forward_as_tuple(args...));
        }

        /**
         * This function exists unimplemented to absorb incorrect template instantiations.
         *
         * @tparam typename swallows the template parameter
         *
         * @param '' swallows arguments
         *
         * @return ignore
         */
        template <typename>
        static bool call_one(...);

        /// The FunctionFusionCaller next step in the recursion
        using NextStep =
            FunctionFusionCaller<std::tuple<Functions...>, Shared, std::tuple<Ranges...>, std::tuple<Arguments...>>;

    public:
        /**
         * Calls the collection of functions with the given arguments.
         *
         * @param args The arguments to be used to call
         *
         * @return A tuple of the returned values, or if the return value was a tuple fuse it
         */
        template <typename... Args>
        static auto call(Args&&... args)
            -> decltype(std::tuple_cat(tuplify(call_one<CurrentFunction>(CurrentRange(), std::forward<Args>(args)...)),
                                       NextStep::call(std::forward<Args>(args)...))) {

            // Call each on a separate line to preserve order of execution
            auto current   = tuplify(call_one<CurrentFunction>(CurrentRange(), std::forward<Args>(args)...));
            auto remainder = NextStep::call(std::forward<Args>(args)...);

            return std::tuple_cat(std::move(current), std::move(remainder));
        }
    };

    /**
     * SFINAE test struct to see if a function is callable with the provided arguments.
     *
     * @tparam Function  The function to be tested
     * @tparam Shared    The number of parameters (from 0) to use in the call
     * @tparam Start     The index of the first argument to pass to the function
     * @tparam End       The index of the element after the last argument to pass to the function
     * @tparam Arguments The types of the arguments passed into the function
     */
    template <typename Function, int Shared, int Start, int End, typename Arguments>
    struct is_callable {
    private:
        using yes = std::true_type;
        using no  = std::false_type;

        template <typename F>
        static auto test(int) -> decltype(apply_function_fusion_call<F, Shared, Start, End>(std::declval<Arguments>()),
                                          yes());
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
     * Splits arguments amongst a series of functions.
     *
     * This is the main loop for the function fusion metafunction.
     * It will do a greedy matching of arguments with function overloads to try and make all functions callable with the
     * given arguments.
     * It will share the first arguments (up to but not including Shared) across all functions.
     *
     * These functions are provided with a wrapper FunctionWrapper which is a template that takes their type and uses
     * its ::call function to call the original functions function.
     * This allows fusion of functions without knowing the name of the function that is being fused.
     *
     * @tparam CurrentFunction    The current function we are inspecting
     * @tparam Functions          The remaining functions we are going to call
     * @tparam Arguments          The arguments we are calling the function with
     * @tparam FunctionWrapper    The template that is used to wrap the Function objects to be called
     * @tparam WrapperArgs        Template types to be used on the FunctionWrapper in addition to the Fuctions type.
     *                            May be empty.
     * @tparam Shared             The number of parameters (from 0) to use in all of the calls
     * @tparam Start              The current attempted index of the first argument to pass
     * @tparam End                The current attempted index of the element after the last argument to pass
     * @tparam ProcessedFunctions The list of functions that have had found argument sets
     * @tparam ArgumentRanges     The respective argument ranges for the processed functions
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
        // Test if we have moved into an invalid range
        : std::conditional_t<
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
     * The termination case for the FunctionFusion metafunction.
     *
     * This is the termination case for the FunctionFusion metafunction.
     * If it reaches this point with a successful combination it will extend from the FunctionFusionCaller.
     * Otherwise it will be a false_type to indicate its failure.
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
        // Check if we used up all of our arguments (and not more than all of our arguments)
        : std::conditional_t<(Start == End && Start == int(sizeof...(Arguments))),
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
