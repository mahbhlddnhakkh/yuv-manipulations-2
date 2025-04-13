cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(myyuv)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# for assert for RelWithDebInfo
# https://stackoverflow.com/questions/22140520/how-to-enable-assert-in-cmake-release-mode/42119938#42119938
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  string( REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  string( REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()

add_subdirectory(myyuv_lib)
add_subdirectory(myyuv_cli)
add_subdirectory(myyuv_opengl)
add_subdirectory(myyuv_sdl3)
