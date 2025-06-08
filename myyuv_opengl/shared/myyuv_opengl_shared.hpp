#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <myyuv.hpp>
#include <vector>

/**
* @brief Callback function for GLFW errors. It just throws exception on this function call.
* @note Register with `glfwSetErrorCallback(glfw_error_callback)`
* @throw std::runtime_error "GLFW error {error}: {desc}"
* @param error Error code.
* @param desc Error description.
*/
void glfw_error_callback(GLint error, const GLchar* desc);

/**
* @brief Enum for figuring out the input image format because BMP and YUV images are handled differently in rendering.
*/
enum class IMAGE_FORMAT { UNKNOWN, BMP, YUV };

/**
* @brief Try to figure out image format based on first 2 bytes. If it's "BM" then it's `BMP`. If it's "YU" then it' `YUV`. Unknown otherwise (error case).
* @note Does not check anything else in the header. Only first 2 bytes.
* @param path Path to input image.
* @return Image format based on first 2 bytes.
*/
IMAGE_FORMAT figure_out_format_magic(const std::string& path);

/**
* @brief Creates and compiles a shader program with vertex and fragment shaders.
* @param[out] shader_program Outputs shader program handler on success.
* @param vertex_shader_src Vertex shader source code.
* @param fragment_shader_src Fragment shader source code.
* @param use_program Whether use the program right after compiling or not. Useful when needed to initialize uniforms later.
* @return 0 if successful, otherwise error code.
*/
int create_shader_program(GLuint& shader_program, const char* vertex_shader_src, const char* fragment_shader_src, bool use_program = true);

/**
* @brief Creates BMP (RGB(A)) texture from `BMP` object.
* @note Puts the texture in specific uniform if `shader_program` is not `0` and `uniform` is not `nullptr`. Otherwise default OpenGL behaviour.
* @param bmp Requested BMP image.
* @param shader_program Shader program to be used in. Ignored if `0` or `uniform` is `nullptr`.
* @param uniform Uniform to put the value in. Ignored if `nullptr` or `shader_program` is `0`.
* @param unit Texture unit. Ignored if `shader_program` is `0` or `uniform` is `nullptr`.
* @return Created texture handler.
*/
GLuint create_bmp_texture(const myyuv::BMP& bmp, GLuint shader_program = 0, const GLchar* uniform = nullptr, GLuint unit = 0);

/**
* @brief Creates YUV textures for each plane from `YUV` object.
* @param yuv Requested YUV image.
* @param shader_program Shader program to be used in.
* @param uniforms Uniforms for each plane.
* @param unit Texture unit for first plane.
* @return Vector of texture handlers.
*/
std::vector<GLuint> create_yuv_texture(const myyuv::YUV& yuv, GLuint shader_program, const std::vector<const GLchar*>& uniforms, GLuint unit = 0);
