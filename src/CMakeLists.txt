FIND_PACKAGE(ZMQ REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(proto_src proto_h "extensions/serialization/NetworkMessage.proto")

INCLUDE_DIRECTORIES(${ZMQ_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})

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

FILE(GLOB src_extensions_serialization
    "extensions/serialization/*.cpp"
    "include/nuclear_bits/extensions/serialization/*.h"
    "include/nuclear_bits/extensions/serialization/*.ipp")
    
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
SOURCE_GROUP(extensions\\serialization  FILES ${proto_src} ${proto_h} ${src_extensions_serialization})

ADD_LIBRARY(nuclear STATIC
    ${src} 
    ${src_threading}
    ${src_extensions}
    ${src_extensions_serialization}
    ${proto_src} ${proto_h}
    ${src_dsl}
    ${src_messages}
    ${src_metaprogramming})
    
TARGET_LINK_LIBRARIES(nuclear ${ZMQ_LIBRARIES} ${PROTOBUF_LIBRARIES})
    
INSTALL(TARGETS nuclear ARCHIVE DESTINATION lib)
