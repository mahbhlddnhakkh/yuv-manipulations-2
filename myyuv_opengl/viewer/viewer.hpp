#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <myyuv.hpp>

void glfw_error_callback(GLint error, const GLchar* desc);

int main_bmp(const myyuv::BMP& bmp);

int main_yuv(const myyuv::YUV& yuv);
