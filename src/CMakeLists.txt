FILE(GLOB src
    "*.cpp"
    "include/nuclear"
    "include/nuclear_bits/*.hpp"
    "include/nuclear_bits/*.ipp")

FILE(GLOB src_threading
    "threading/*.cpp"
    "include/nuclear_bits/threading/*.hpp"
    "include/nuclear_bits/threading/*.ipp")

FILE(GLOB src_extension
    "extension/*.cpp"
    "include/nuclear_bits/extension/*.hpp"
    "include/nuclear_bits/extension/*.ipp")

FILE(GLOB src_extension_network
    "extension/network/*.cpp"
    "include/nuclear_bits/extension/network/*.hpp"
    "include/nuclear_bits/extension/network/*.ipp")

FILE(GLOB src_dsl
    "dsl/*.cpp"
    "include/nuclear_bits/dsl/*.hpp"
    "include/nuclear_bits/dsl/*.ipp")

FILE(GLOB src_dsl_store
    "dsl/store/*.cpp"
    "include/nuclear_bits/dsl/store/*.hpp"
    "include/nuclear_bits/dsl/store/*.ipp")

FILE(GLOB src_dsl_trait
    "dsl/trait/*.cpp"
    "include/nuclear_bits/dsl/trait/*.hpp"
    "include/nuclear_bits/dsl/trait/*.ipp")

FILE(GLOB src_dsl_operation
    "dsl/operation/*.cpp"
    "include/nuclear_bits/dsl/operation/*.hpp"
    "include/nuclear_bits/dsl/operation/*.ipp")

FILE(GLOB src_dsl_fusion
    "dsl/fusion/*.cpp"
    "include/nuclear_bits/dsl/fusion/*.hpp"
    "include/nuclear_bits/dsl/fusion/*.ipp")

FILE(GLOB src_dsl_validation
    "dsl/validation/*.cpp"
    "include/nuclear_bits/dsl/validation/*.hpp"
    "include/nuclear_bits/dsl/validation/*.ipp")

FILE(GLOB src_dsl_word
    "dsl/word/*.cpp"
    "include/nuclear_bits/dsl/word/*.hpp"
    "include/nuclear_bits/dsl/word/*.ipp")

FILE(GLOB src_dsl_word_emit
    "dsl/word/emit/*.cpp"
    "include/nuclear_bits/dsl/word/emit/*.hpp"
    "include/nuclear_bits/dsl/word/emit/*.ipp")

FILE(GLOB src_message
    "messages/*.cpp"
    "include/nuclear_bits/message/*.hpp"
    "include/nuclear_bits/message/*.ipp")

FILE(GLOB src_util
    "util/*.cpp"
    "include/nuclear_bits/util/*.hpp"
    "include/nuclear_bits/util/*.ipp")

FILE(GLOB src_util_network
    "util/network/*.cpp"
    "include/nuclear_bits/util/network/*.hpp"
    "include/nuclear_bits/util/network/*.ipp")

FILE(GLOB src_util_serialise
    "util/serialise/*.c"
    "util/serialise/*.cpp"
    "include/nuclear_bits/util/serialise/*.h"
    "include/nuclear_bits/util/serialise/*.hpp"
    "include/nuclear_bits/util/serialise/*.ipp")

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

INSTALL(TARGETS nuclear ARCHIVE DESTINATION lib)
