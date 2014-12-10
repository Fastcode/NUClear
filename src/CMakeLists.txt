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

FILE(GLOB src_messages
    "messages/*.cpp"
    "include/nuclear_bits/messages/*.h"
    "include/nuclear_bits/messages/*.ipp")

FILE(GLOB src_metaprogramming
    "metaprogramming/*.cpp"
    "include/nuclear_bits/metaprogramming/*.h"
    "include/nuclear_bits/metaprogramming/*.ipp")

SOURCE_GROUP(""                         FILES ${src})
SOURCE_GROUP(threading                  FILES ${src_threading})
SOURCE_GROUP(extensions                 FILES ${src_extensions})
SOURCE_GROUP(dsl                        FILES ${src_dsl})
SOURCE_GROUP(messages                   FILES ${src_messages})
SOURCE_GROUP(metaprogramming            FILES ${src_metaprogramming})

ADD_LIBRARY(nuclear STATIC
    ${src} 
    ${src_threading}
    ${src_extensions}
    ${src_extensions_serialization}
    ${proto_src} ${proto_h}
    ${src_dsl}
    ${src_messages}
    ${src_metaprogramming})
    
INSTALL(TARGETS nuclear ARCHIVE DESTINATION lib)
