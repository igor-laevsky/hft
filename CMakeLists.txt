cmake_minimum_required(VERSION 3.25)
project(hft)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti -Werror -Wall -Wextra -pedantic")

include_directories(src/)

# libcurl
message(STATUS ${CMAKE_PREFIX_PATH})

find_library(LIBCURL_LIBRARY NAMES curl libcurl)
find_path(LIBCURL_INCLUDE_DIR NAMES curl.h PATH_SUFFIXES curl)

if(LIBCURL_LIBRARY AND LIBCURL_INCLUDE_DIR)
    message(STATUS "Found libcurl: ${LIBCURL_LIBRARY}")
    message(STATUS "libcurl includes: ${LIBCURL_INCLUDE_DIR}")
else()
    message(FATAL_ERROR "Could not find libelf. Please install libelf development package.")
endif()
include_directories(${LIBCURL_INCLUDE_DIR})

# GTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(sources
        src/coinbase_ticker.cpp
        src/coinbase_ticker.hpp
        )

set(tests
        test/fst.cpp)

add_executable(hft
        src/main.cpp
        ${sources})
target_link_libraries(hft ${LIBCURL_LIBRARY})

add_executable(hft_tests ${sources} ${tests})
target_link_libraries(hft_tests gtest_main)
include(GoogleTest)
gtest_discover_tests(hft_tests)
