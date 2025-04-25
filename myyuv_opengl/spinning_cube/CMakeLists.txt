cmake_minimum_required(VERSION 3.16)

project(myyuv_opengl_spinning_cube LANGUAGES CXX)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${PROJECT_NAME} PRIVATE main.cpp spinning_cube.cpp spinning_cube_bmp.cpp spinning_cube_yuv.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE myyuv_opengl_shared myyuv_lib glfw GLEW::glew OpenGL::GL glm::glm)
