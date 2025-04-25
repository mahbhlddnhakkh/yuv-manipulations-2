#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <myyuv.hpp>
#include <vector>

void glfw_error_callback(GLint error, const GLchar* desc);

enum class IMAGE_FORMAT { UNKNOWN, BMP, YUV };

IMAGE_FORMAT figure_out_format_magic(const std::string& path);

int create_shader_program(GLuint& shader_program, const char* vertex_shader_src, const char* fragment_shader_src, bool use_program = true);

GLuint create_bmp_texture(const myyuv::BMP& bmp, GLuint shader_program = 0, const GLchar* uniform = nullptr, GLuint unit = 0);

std::vector<GLuint> create_yuv_texture(const myyuv::YUV& yuv, GLuint shader_program, const std::vector<const GLchar*>& uniforms, GLuint unit = 0);
