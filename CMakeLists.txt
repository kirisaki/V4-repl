cmake_minimum_required(VERSION 3.15)
project(
  v4-repl
  VERSION 0.2.0
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(WITH_FILESYSTEM "Enable filesystem support (history file)" ON)
set(V4_LOCAL_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/../V4"
    CACHE PATH "Path to local V4 source")
set(V4FRONT_LOCAL_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/../V4-front"
    CACHE PATH "Path to local V4-front source")

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions")

# FetchContent setup
include(FetchContent)

# V4-front (will automatically include V4 via V4_SRC_DIR)
if(EXISTS "${V4FRONT_LOCAL_PATH}/CMakeLists.txt")
  message(STATUS "Using local V4-front from: ${V4FRONT_LOCAL_PATH}")
  set(V4_SRC_DIR
      "${V4_LOCAL_PATH}"
      CACHE PATH "Path to V4 source for V4-front")
  add_subdirectory(${V4FRONT_LOCAL_PATH} ${CMAKE_BINARY_DIR}/v4front)
else()
  message(STATUS "Fetching V4-front from repository")
  set(V4_FETCH
      ON
      CACHE BOOL "Fetch V4 from Git" FORCE)
  FetchContent_Declare(
    v4front
    GIT_REPOSITORY https://github.com/kirisaki/V4-front.git
    GIT_TAG main)
  FetchContent_MakeAvailable(v4front)
endif()

# doctest (for libv4repl tests)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG master)
FetchContent_MakeAvailable(doctest)

# linenoise (Unix only - requires termios.h)
if(UNIX)
  FetchContent_Declare(
    linenoise
    GIT_REPOSITORY https://github.com/antirez/linenoise.git
    GIT_TAG master)
  FetchContent_MakeAvailable(linenoise)

  # linenoise is a C library, create a wrapper target
  add_library(linenoise_lib STATIC ${linenoise_SOURCE_DIR}/linenoise.c)
  target_include_directories(linenoise_lib PUBLIC ${linenoise_SOURCE_DIR})
endif()

# V4-REPL library (platform-independent C API)
add_library(v4repl STATIC src/repl.c)

target_include_directories(
  v4repl
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(v4repl PUBLIC v4vm v4front)

# Main executable (C++ REPL with linenoise - Unix only)
if(UNIX)
  add_executable(v4-repl src/main.cpp src/repl.cpp src/meta_commands.cpp)

  target_include_directories(v4-repl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

  target_link_libraries(v4-repl PRIVATE v4vm v4front linenoise_lib mock_hal)

  # Conditional compilation for filesystem support
  if(WITH_FILESYSTEM)
    target_compile_definitions(v4-repl PRIVATE WITH_FILESYSTEM=1)
  endif()

  message(STATUS "Building v4-repl executable (Unix only)")
else()
  message(STATUS "Skipping v4-repl executable (requires Unix/linenoise)")
endif()

# libv4repl tests (using doctest)
add_executable(test_libv4repl tests/test_libv4repl.cpp)
target_link_libraries(test_libv4repl PRIVATE v4repl v4vm doctest::doctest
                                             mock_hal)
target_include_directories(test_libv4repl
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Installation
if(UNIX)
  install(TARGETS v4-repl v4repl DESTINATION bin)
else()
  install(TARGETS v4repl DESTINATION bin)
endif()
install(DIRECTORY include/v4repl DESTINATION include)
