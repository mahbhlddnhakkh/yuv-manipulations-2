cmake_minimum_required(VERSION 3.16)

project(myyuv_opengl_viewer LANGUAGES CXX)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${PROJECT_NAME} PRIVATE main.cpp viewer.cpp viewer_bmp.cpp viewer_yuv.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE myyuv_lib glfw GLEW::glew OpenGL::GL)
