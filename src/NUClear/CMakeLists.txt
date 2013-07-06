FIND_PACKAGE(ZMQ REQUIRED)
FIND_PACKAGE(Protobuf REQUIRED)

PROTOBUF_GENERATE_CPP(proto_src proto_h Networking/NetworkMessage.proto)

include_directories(${ZMQ_INCLUDE_DIRS})
include_directories(${PROTOBUF_INCLUDE_DIRS})

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

source_group(NUClear FILES ${src} ${include_public})
source_group(NUClear\\Networking FILES ${src_networking} ${proto_src} ${proto_h})
source_group(NUClear\\Internal FILES ${src_internal})
source_group(NUClear\\Internal\\Magic FILES ${src_internal_magic})
source_group(NUClear\\Internal\\CommandTypes FILES ${src_internal_commandtypes})
source_group(NUClear\\Internal\\Extensions FILES ${src_internal_extensions})

add_library(nuclear 
    ${include_public} 
    ${src} 
    ${src_networking}
    ${proto_src}
    ${proto_h}
    ${src_internal}
    ${src_internal_commandtypes}
    ${src_internal_extensions}
    ${src_internal_magic})
