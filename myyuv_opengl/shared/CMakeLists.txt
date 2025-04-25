cmake_minimum_required(VERSION 3.16)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm CONFIG REQUIRED)

project(myyuv_opengl_shared LANGUAGES CXX)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)

set(MY_SRC_FILES
  myyuv_opengl_shared.hpp
  myyuv_opengl_shared.cpp
)

add_library(${PROJECT_NAME} SHARED)
target_include_directories(${PROJECT_NAME} PUBLIC .)
target_sources(${PROJECT_NAME} PRIVATE ${MY_SRC_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER myyuv_opengl_shared.hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE myyuv_lib glfw GLEW::glew OpenGL::GL glm::glm)
