#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <myyuv.hpp>

void create_rectangle(GLuint& VAO, GLuint& VBO, GLuint& EBO);

void handle_events(GLFWwindow* window);

int main_bmp(const myyuv::BMP& bmp);

int main_yuv(myyuv::YUV yuv);
