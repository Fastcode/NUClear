# Default not to run the clang-tidy checks, default to whatever our CI_BUILD is
option(ENABLE_CLANG_TIDY "Enable building with clang-tidy checks.")
if(ENABLE_CLANG_TIDY)
  # Search for clang-tidy-15 first as this is the version installed in CI
  find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy-15 clang-tidy)
  if(NOT CLANG_TIDY_EXECUTABLE)
    message(FATAL_ERROR "clang-tidy-15 not found.")
  endif()

  # Report clang-tidy executable details
  execute_process(COMMAND "${CLANG_TIDY_EXECUTABLE}" "--version" OUTPUT_VARIABLE CLANG_TIDY_VERSION)
  string(REGEX REPLACE ".*LLVM version ([0-9.]*).*" "\\1" CLANG_TIDY_VERSION "${CLANG_TIDY_VERSION}")
  message(STATUS "Found clang-tidy: ${CLANG_TIDY_EXECUTABLE} ${CLANG_TIDY_VERSION}")

  # Build clang-tidy command line
  set(CLANG_TIDY_ARGS "${CLANG_TIDY_EXECUTABLE}" "--use-color" "--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy")
  if(CI_BUILD)
    set(CLANG_TIDY_ARGS "${CLANG_TIDY_EXECUTABLE}" "-warnings-as-errors=*")
  endif(CI_BUILD)
  set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_ARGS}" "--extra-arg=-std=c++14")
  set(CMAKE_C_CLANG_TIDY "${CLANG_TIDY_ARGS}" "--extra-arg=-std=c99")
endif(ENABLE_CLANG_TIDY)
