FILE(GLOB src
    "nuclear"
    "*.cpp"
    "*.hpp"
    "*.ipp")

FILE(GLOB src_threading
    "threading/*.cpp"
    "threading/*.hpp"
    "threading/*.ipp")

FILE(GLOB src_extension
    "extension/*.cpp"
    "extension/*.hpp"
    "extension/*.ipp")

FILE(GLOB src_extension_network
    "extension/network/*.cpp"
    "extension/network/*.hpp"
    "extension/network/*.ipp")

FILE(GLOB src_dsl
    "dsl/*.cpp"
    "dsl/*.hpp"
    "dsl/*.ipp")

FILE(GLOB src_dsl_store
    "dsl/store/*.cpp"
    "dsl/store/*.hpp"
    "dsl/store/*.ipp")

FILE(GLOB src_dsl_trait
    "dsl/trait/*.cpp"
    "dsl/trait/*.hpp"
    "dsl/trait/*.ipp")

FILE(GLOB src_dsl_operation
    "dsl/operation/*.cpp"
    "dsl/operation/*.hpp"
    "dsl/operation/*.ipp")

FILE(GLOB src_dsl_fusion
    "dsl/fusion/*.cpp"
    "dsl/fusion/*.hpp"
    "dsl/fusion/*.ipp")

FILE(GLOB src_dsl_validation
    "dsl/validation/*.cpp"
    "dsl/validation/*.hpp"
    "dsl/validation/*.ipp")

FILE(GLOB src_dsl_word
    "dsl/word/*.cpp"
    "dsl/word/*.hpp"
    "dsl/word/*.ipp")

FILE(GLOB src_dsl_word_emit
    "dsl/word/emit/*.cpp"
    "dsl/word/emit/*.hpp"
    "dsl/word/emit/*.ipp")

FILE(GLOB src_message
    "messages/*.cpp"
    "message/*.hpp"
    "message/*.ipp")

FILE(GLOB src_util
    "util/*.cpp"
    "util/*.hpp"
    "util/*.ipp")

FILE(GLOB src_util_network
    "util/network/*.cpp"
    "util/network/*.hpp"
    "util/network/*.ipp")

FILE(GLOB src_util_serialise
    "util/serialise/*.c"
    "util/serialise/*.cpp"
    "util/serialise/*.h"
    "util/serialise/*.hpp"
    "util/serialise/*.ipp")

SOURCE_GROUP(""                         FILES ${src})
SOURCE_GROUP(threading                  FILES ${src_threading})
SOURCE_GROUP(extension                  FILES ${src_extension})
SOURCE_GROUP(extension\\network         FILES ${src_extension_network})
SOURCE_GROUP(dsl                        FILES ${src_dsl})
SOURCE_GROUP(dsl\\store                 FILES ${src_dsl_store})
SOURCE_GROUP(dsl\\trait                 FILES ${src_dsl_trait})
SOURCE_GROUP(dsl\\operation             FILES ${src_dsl_operation})
SOURCE_GROUP(dsl\\fusion                FILES ${src_dsl_fusion})
SOURCE_GROUP(dsl\\validation            FILES ${src_dsl_validation})
SOURCE_GROUP(dsl\\word                  FILES ${src_dsl_word})
SOURCE_GROUP(dsl\\word\\emit            FILES ${src_dsl_word_emit})
SOURCE_GROUP(message                    FILES ${src_message})
SOURCE_GROUP(util                       FILES ${src_util})
SOURCE_GROUP(util\\network              FILES ${src_util_network})
SOURCE_GROUP(util\\serialise            FILES ${src_util_serialise})

FIND_PACKAGE(Threads REQUIRED)

ADD_LIBRARY(nuclear STATIC
    ${src}
    ${src_threading}
    ${src_extension}
    ${src_extension_network}
    ${src_dsl_store}
    ${src_dsl_trait}
    ${src_dsl_operation}
    ${src_dsl_fusion}
    ${src_dsl_validation}
    ${src_dsl_word}
    ${src_dsl_word_emit}
    ${src_dsl}
    ${src_message}
    ${src_util}
    ${src_util_network}
    ${src_util_serialise})
SET_TARGET_PROPERTIES(nuclear PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

# This is where we set the features required by NUClear
TARGET_COMPILE_FEATURES(nuclear PUBLIC
    cxx_constexpr
    cxx_decltype
    cxx_deleted_functions
    cxx_generic_lambdas
    cxx_inheriting_constructors
    cxx_lambdas
    cxx_lambda_init_captures
    cxx_noexcept
    cxx_nonstatic_member_init
    cxx_nullptr
    cxx_range_for
    cxx_return_type_deduction
    cxx_right_angle_brackets
    cxx_rvalue_references
    cxx_static_assert
    cxx_thread_local
    cxx_trailing_return_types
    cxx_uniform_initialization
    cxx_variadic_templates
    cxx_template_template_parameters
)

TARGET_LINK_LIBRARIES(nuclear ${CMAKE_THREAD_LIBS_INIT})

# Install the headers and templates and libraries
INSTALL(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/ 
    DESTINATION include/nuclear_bits
    PATTERN "*.hpp" PATTERN "*.ipp" PATTERN "*.h"
)
INSTALL(TARGETS nuclear ARCHIVE DESTINATION lib)
