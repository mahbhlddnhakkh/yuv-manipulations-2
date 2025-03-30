cmake_minimum_required(VERSION 3.16)

project(myyuv_lib LANGUAGES CXX)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

set(MY_SRC_FILES
  myyuv.hpp
  myyuv_bmp.hpp
  myyuv_bmp.cpp
  myyuv_yuv.hpp
  myyuv_yuv.cpp
)

add_library(${PROJECT_NAME} SHARED)
target_include_directories(${PROJECT_NAME} PUBLIC .)
target_sources(${PROJECT_NAME} PRIVATE ${MY_SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER myyuv.hpp)
