FILE(GLOB src "*.cpp"
    "Internal/*.cpp")

FILE(GLOB inc "*.h"
    "Internal/*.h"
    "Internal/Magic/*.h")

FILE(GLOB ipp "*.ipp"
    "Internal/*.ipp"
    "Internal/Magic/*.ipp")

add_library(nuclear ${src} ${inc} ${ipp})
