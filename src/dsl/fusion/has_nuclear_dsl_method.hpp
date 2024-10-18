#ifndef NUCLEAR_DSL_FUSION_FUSION_HAS_HPP
#define NUCLEAR_DSL_FUSION_FUSION_HAS_HPP

#include "NoOp.hpp"

/**
 * This macro to create an SFINAE type to check for the existence of a NUClear DSL method in a given class.
 *
 * The macro generates a template struct that can be used to determine if a class contains a specific method.
 *
 * The macro works by attempting to instantiate a function pointer to the method in question.
 * If the method exists, the instantiation will succeed and the struct will inherit from
 * `std::true_type`. Otherwise, it will inherit from `std::false_type`.
 *
 * @note This macro assumes that the method being checked for is a template method that can be instantiated with
 * `ParsedNoOp`.
 */
#define HAS_NUCLEAR_DSL_METHOD(Method)                                                 \
    template <typename Word>                                                           \
    struct has_##Method {                                                              \
    private:                                                                           \
        template <typename R, typename... A>                                           \
        static auto test_func(R (*)(A...)) -> std::true_type;                          \
        static auto test_func(...) -> std::false_type;                                 \
                                                                                       \
        template <typename U>                                                          \
        static auto test(int) -> decltype(test_func(&U::template Method<ParsedNoOp>)); \
        template <typename>                                                            \
        static auto test(...) -> std::false_type;                                      \
                                                                                       \
    public:                                                                            \
        static constexpr bool value = decltype(test<Word>(0))::value;                  \
    }

#endif  // NUCLEAR_DSL_FUSION_FUSION_HAS_HPP
