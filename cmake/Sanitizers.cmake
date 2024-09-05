###############################################################################
### Options for enabling different sanitisers.                              ###
###                                                                         ###
### User beware:                                                            ###
### Not all sanitisers can be enabled at the same time.                     ###
###############################################################################

option(SANITIZE_ADDRESS "Enable AddressSanitizer." OFF)
option(SANITIZE_LEAK "Enable Leak Sanitizer." OFF)
option(SANITIZE_MEMORY "Enable MemorySanitizer." OFF)
option(SANITIZE_THREAD "Enable ThreadSanitizer." OFF)
option(SANITIZE_UNDEFINED "Enable UndefinedBehaviourSanitizer." OFF)

if(SANITIZE_ADDRESS AND (SANITIZE_THREAD OR SANITIZE_MEMORY))
  message(FATAL_ERROR "AddressSanitizer is not compatible with " "ThreadSanitizer or MemorySanitizer.")
endif()

if(SANITIZE_ADDRESS)
  if(MSVC)
    add_compile_options("/fsanitize=address")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-g" "-fsanitize=address" "-fno-omit-frame-pointer")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-g" "-fsanitize=address")
  else()
    message(FATAL_ERROR "Unsupported compiler")
  endif()
endif()

if(SANITIZE_LEAK)
  if(MSVC)
    add_compile_options("/fsanitize=leak")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-g" "-fsanitize=leak")
  else()
    message(FATAL_ERROR "Unsupported compiler")
  endif()
endif()

if(SANITIZE_MEMORY)
  if(MSVC)
    add_compile_options("/fsanitize=memory")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-g" "-fsanitize=memory")
  else()
    message(FATAL_ERROR "Unsupported compiler")
  endif()
endif()

if(SANITIZE_THREAD)
  if(MSVC)
    add_compile_options("/fsanitize=thread")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-g" "-fsanitize=thread")
  else()
    message(FATAL_ERROR "Unsupported compiler")
  endif()
endif()

if(SANITIZE_UNDEFINED)
  if(MSVC)
    add_compile_options("/fsanitize=undefined")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options("-g" "-fsanitize=undefined")
  else()
    message(FATAL_ERROR "Unsupported compiler")
  endif()
endif()

# Option for enabling code profiling. Disabled by default
option(ENABLE_PROFILING "Compile with profiling support enabled." OFF)
if(ENABLE_PROFILING)
  if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message(
      WARNING
        "Profiling is enabled but no debugging symbols will be kept in the compiled binaries. This may cause fine-grained profilling data to be lost."
    )
  endif()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg -fprofile-arcs")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fprofile-arcs")
  set(CMAKE_LINKER "${CMAKE_LINKER_FLAGS} -pg -fprofile-arcs")
endif(ENABLE_PROFILING)
