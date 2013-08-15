FIND_PACKAGE(ZMQ REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(proto_src proto_h Serialization/NetworkMessage.proto)

INCLUDE_DIRECTORIES(${ZMQ_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})

FILE(GLOB src 
    "*.cpp"
    "${PROJECT_SOURCE_DIR}/include/NUClear/*.h"
    "${PROJECT_SOURCE_DIR}/include/NUClear/*.ipp")

FILE(GLOB src_serialization
    "${PROJECT_SOURCE_DIR}/src/NUClear/Serialization/*.proto"
    "${PROJECT_SOURCE_DIR}/src/NUClear/Serialization/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Serialization/*.h"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Serialization/*.ipp") 

FILE(GLOB src_internal 
    "${PROJECT_SOURCE_DIR}/src/NUClear/Internal/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Internal/*.h"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Internal/*.ipp") 
    
FILE(GLOB src_internal_commandtypes
    "${PROJECT_SOURCE_DIR}/src/NUClear/Internal/CommandTypes/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Internal/CommandTypes/*.h"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Internal/CommandTypes/*.ipp")
    
FILE(GLOB src_extensions
    "${PROJECT_SOURCE_DIR}/src/NUClear/Extensions/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Extensions/*.h"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Extensions/*.ipp") 

FILE(GLOB src_internal_magic 
    "${PROJECT_SOURCE_DIR}/src/NUClear/Internal/Magic/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Internal/Magic/*.h"
    "${PROJECT_SOURCE_DIR}/include/NUClear/Internal/Magic/*.ipp")

FILE(GLOB include_public
    "${PROJECT_SOURCE_DIR}/src/NUClear/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/NUClear/*.h"
    "${PROJECT_SOURCE_DIR}/include/NUClear/*.ipp")

SOURCE_GROUP(NUClear                            FILES ${src} ${include_public})
SOURCE_GROUP(NUClear\\Serialization             FILES ${src_serialization} ${proto_src} ${proto_h})
SOURCE_GROUP(NUClear\\Extensions                FILES ${src_extensions})
SOURCE_GROUP(NUClear\\Internal                  FILES ${src_internal})
SOURCE_GROUP(NUClear\\Internal\\Magic           FILES ${src_internal_magic})
SOURCE_GROUP(NUClear\\Internal\\CommandTypes    FILES ${src_internal_commandtypes})

ADD_LIBRARY(nuclear STATIC
    ${include_public} 
    ${src} 
    ${src_serialization}
    ${proto_src}
    ${proto_h}
    ${src_extensions}
    ${src_internal}
    ${src_internal_commandtypes}
    ${src_internal_magic})
    
TARGET_LINK_LIBRARIES(nuclear ${ZMQ_LIBRARIES} ${PROTOBUF_LIBRARIES})
    
INSTALL(TARGETS nuclear ARCHIVE DESTINATION lib)
