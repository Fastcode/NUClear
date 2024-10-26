
#ifndef NUCLEAR_DSL_FUSION_FUSION_IS_DSL_WORD_HPP
#define NUCLEAR_DSL_FUSION_FUSION_IS_DSL_WORD_HPP

#include <tuple>
#include <type_traits>

#include "../../../threading/ReactionTask.hpp"
#include "Caller.hpp"

namespace NUClear {
namespace dsl {
    namespace fusion {

        /**
         * Determines if a given type is a DSL word for the given hook.
         *
         * @tparam Hook the template type which wraps a specific hook
         * @tparam Word the type to check
         */
        template <template <typename> class Hook>
        struct is_dsl_word {
            template <typename Word>
            struct check {
            private:
                using yes = std::true_type;
                using no  = std::false_type;

                template <typename R, typename... A>
                static yes test_func(R (*)(threading::ReactionTask&, A...));
                static no test_func(...);

                template <typename U>
                static auto test(int) -> decltype(test_func(Caller<Hook>::template call<U, void>));
                template <typename>
                static no test(...);

            public:
                static constexpr bool value = std::is_same<decltype(test<Word>(0)), yes>::value;
            };
        };

    }  // namespace fusion
}  // namespace dsl
}  // namespace NUClear

#endif  // NUCLEAR_DSL_FUSION_FUSION_IS_DSL_WORD_HPP
