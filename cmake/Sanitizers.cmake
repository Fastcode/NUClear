###############################################################################
### Options for enabling different sanitisers.                              ###
###                                                                         ###
### User beware:                                                            ###
### Not all sanitisers can be enabled at the same time.                     ###
###############################################################################
option(USE_ASAN "Enable address sanitization" OFF)
if(USE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -U_FORTIFY_SOURCE -fno-common)
  add_link_options(-fsanitize=address)
  link_libraries(asan)
endif(USE_ASAN)

option(USE_LSAN "Enable leak sanitization" OFF)
if(USE_LSAN)
  add_compile_options(-fsanitize=leak -fno-omit-frame-pointer -U_FORTIFY_SOURCE -fno-common)
  add_link_options(-fsanitize=leak)
  link_libraries(lsan)
endif(USE_LSAN)

option(USE_TSAN "Enable thread sanitization" OFF)
if(USE_TSAN)
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -U_FORTIFY_SOURCE -fno-common)
  add_link_options(-fsanitize=thread)
  link_libraries(tsan)
endif(USE_TSAN)

option(USE_UBSAN "Enable undefined behaviour sanitization" OFF)
if(USE_UBSAN)
  add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer -U_FORTIFY_SOURCE -fno-common)
  add_link_options(-fsanitize=undefined)
  link_libraries(ubsan)
endif(USE_UBSAN)

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
