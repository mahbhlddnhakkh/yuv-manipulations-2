cmake_minimum_required(VERSION 3.16)

project(myyuv_lib LANGUAGES CXX)

option(MYYUV_USE_OPENMP "Use OpenMP in YUV compression and decompression" OFF)

if(MYYUV_USE_OPENMP)
  find_package(OpenMP REQUIRED)
  find_package(Threads REQUIRED)
endif(MYYUV_USE_OPENMP)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

set(MY_SRC_FILES
  myyuv.hpp
  myyuv_bmp.hpp
  myyuv_bmp.cpp
  myyuv_yuv.hpp
  myyuv_yuv.cpp
  myyuv_DCT/DCT.cpp
  myyuv_DCT/Huffman.cpp
)

add_library(${PROJECT_NAME} SHARED)
target_include_directories(${PROJECT_NAME} PUBLIC .)
target_sources(${PROJECT_NAME} PRIVATE ${MY_SRC_FILES})

if(MYYUV_USE_OPENMP)
  target_compile_definitions(${PROJECT_NAME} PRIVATE MYYUV_USE_OPENMP)
  target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads OpenMP::OpenMP_CXX)
endif(MYYUV_USE_OPENMP)

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER myyuv.hpp)
