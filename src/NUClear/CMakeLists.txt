FIND_PACKAGE(ZMQ REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(proto_src proto_h Networking/NetworkMessage.proto)

INCLUDE_DIRECTORIES(${ZMQ_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${PROTOBUF_INCLUDE_DIRS})

FILE(GLOB src 
    "*.cpp"
    "*.h"
    "*.ipp")

FILE(GLOB src_networking 
    "Networking/*.h"
    "Networking/*.cpp"
    "Networking/*.cc"
    "Networking/*.ipp"
    "Networking/*.proto") 

FILE(GLOB src_internal 
    "Internal/*.h"
    "Internal/*.cpp"
    "Internal/*.ipp") 
    
FILE(GLOB src_internal_commandtypes
    "Internal/CommandTypes/*.h"
    "Internal/CommandTypes/*.cpp"
    "Internal/CommandTypes/*.ipp")
    
FILE(GLOB src_internal_extensions
    "Internal/Extensions/*.h"
    "Internal/Extensions/*.cpp"
    "Internal/Extensions/*.ipp") 

FILE(GLOB src_internal_magic 
    "Internal/Magic/*.h"
    "Internal/Magic/*.cpp"
    "Internal/Magic/*.ipp")

FILE(GLOB include_public
    "${PROJECT_SOURCE_DIR}/include/NUClear/*.h"
    "${PROJECT_SOURCE_DIR}/include/NUClear/*.cpp"
    "${PROJECT_SOURCE_DIR}/include/NUClear/*.ipp")

SOURCE_GROUP(NUClear                            FILES ${src} ${include_public})
SOURCE_GROUP(NUClear\\Networking                FILES ${src_networking} ${proto_src} ${proto_h})
SOURCE_GROUP(NUClear\\Internal                  FILES ${src_internal})
SOURCE_GROUP(NUClear\\Internal\\Magic           FILES ${src_internal_magic})
SOURCE_GROUP(NUClear\\Internal\\CommandTypes    FILES ${src_internal_commandtypes})
SOURCE_GROUP(NUClear\\Internal\\Extensions      FILES ${src_internal_extensions})

ADD_LIBRARY(nuclear SHARED
    ${include_public} 
    ${src} 
    ${src_networking}
    ${proto_src}
    ${proto_h}
    ${src_internal}
    ${src_internal_commandtypes}
    ${src_internal_extensions}
    ${src_internal_magic})
    
TARGET_LINK_LIBRARIES(nuclear ${ZMQ_LIBRARIES} ${PROTOBUF_LIBRARIES})
    
INSTALL(TARGETS nuclear LIBRARY DESTINATION lib)
