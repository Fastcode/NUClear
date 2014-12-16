FILE(GLOB src
    "*.cpp"
    "include/nuclear"
    "include/nuclear_bits/*.h"
    "include/nuclear_bits/*.ipp")

FILE(GLOB src_threading
    "threading/*.cpp"
    "include/nuclear_bits/threading/*.h"
    "include/nuclear_bits/threading/*.ipp")

FILE(GLOB src_extensions
    "extensions/*.cpp"
    "include/nuclear_bits/extensions/*.h"
    "include/nuclear_bits/extensions/*.ipp")
    
FILE(GLOB src_dsl
    "dsl/*.cpp"
    "include/nuclear_bits/dsl/*.h"
    "include/nuclear_bits/dsl/*.ipp")

FILE(GLOB src_dsl_store
    "dsl/store/*.cpp"
    "include/nuclear_bits/dsl/store/*.h"
    "include/nuclear_bits/dsl/store/*.ipp")

FILE(GLOB src_dsl_operation
    "dsl/operation/*.cpp"
    "include/nuclear_bits/dsl/operation/*.h"
    "include/nuclear_bits/dsl/operation/*.ipp")

FILE(GLOB src_dsl_word
    "dsl/word/*.cpp"
    "include/nuclear_bits/dsl/word/*.h"
    "include/nuclear_bits/dsl/word/*.ipp")

FILE(GLOB src_messages
    "messages/*.cpp"
    "include/nuclear_bits/messages/*.h"
    "include/nuclear_bits/messages/*.ipp")

FILE(GLOB src_util
    "util/*.cpp"
    "include/nuclear_bits/util/*.h"
    "include/nuclear_bits/util/*.ipp")

SOURCE_GROUP(""                         FILES ${src})
SOURCE_GROUP(threading                  FILES ${src_threading})
SOURCE_GROUP(extensions                 FILES ${src_extensions})
SOURCE_GROUP(dsl                        FILES ${src_dsl})
SOURCE_GROUP(dsl\\store                 FILES ${src_dsl_store})
SOURCE_GROUP(dsl\\operation             FILES ${src_dsl_operation})
SOURCE_GROUP(dsl\\word                  FILES ${src_dsl_word})
SOURCE_GROUP(messages                   FILES ${src_messages})
SOURCE_GROUP(util                       FILES ${src_util})

ADD_LIBRARY(nuclear STATIC
    ${src} 
    ${src_threading}
    ${src_extensions}
    ${src_dsl_store}
    ${src_dsl_operation}
    ${src_dsl_word}
    ${src_dsl}
    ${src_messages}
    ${src_util})
    
INSTALL(TARGETS nuclear ARCHIVE DESTINATION lib)
