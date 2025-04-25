#include "myyuv_opengl_shared.hpp"

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <cassert>

void glfw_error_callback(GLint error, const GLchar* desc) {
  //std::cout << "GLFW error " << error << ": " << desc << '\n';
  throw std::runtime_error(std::string("GLFW error " + std::to_string(error) + ": " + desc));
}

IMAGE_FORMAT figure_out_format_magic(const std::string& path) {
  IMAGE_FORMAT format = IMAGE_FORMAT::UNKNOWN;
  myyuv::BMPHeader bmp_header;
  myyuv::YUVHeader yuv_header;
  const uint32_t magic_size = std::max(sizeof(bmp_header.type), sizeof(yuv_header.type));
  uint8_t magic[magic_size];
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Error opening file to read " + path);
  }
  f.read(reinterpret_cast<char*>(magic), sizeof(magic));
  f.close();
  int ret = 0;
  if (std::equal(bmp_header.type, bmp_header.type + sizeof(bmp_header.type), magic)) {
    format = IMAGE_FORMAT::BMP;
  } else if (std::equal(yuv_header.type, yuv_header.type + sizeof(yuv_header.type), magic)) {
    format = IMAGE_FORMAT::YUV;
  } else {
    format = IMAGE_FORMAT::UNKNOWN;
  }
  return format;
}

int create_shader_program(GLuint& shader_program, const char* vertex_shader_src, const char* fragment_shader_src, bool use_program) {
  assert(vertex_shader_src);
  assert(fragment_shader_src);
  shader_program = 0;
  GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_src, nullptr);
  glCompileShader(vertex_shader);
  GLint shader_compile_success;
  GLchar shader_compile_log[512];
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shader_compile_success);
  if (!shader_compile_success) {
    glGetShaderInfoLog(vertex_shader, 512, nullptr, shader_compile_log);
    std::cout << "Error. Failed to compile vertex shader\n" << shader_compile_log << '\n';
    return -1;
  }
  GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_src, nullptr);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shader_compile_success);
  if (!shader_compile_success) {
    glGetShaderInfoLog(fragment_shader, 512, nullptr, shader_compile_log);
    std::cout << "Error. Failed to compile fragment shader\n" << shader_compile_log << '\n';
    return -1;
  }
  shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  glGetShaderiv(shader_program, GL_LINK_STATUS, &shader_compile_success);
  if (!shader_compile_success) {
    glGetShaderInfoLog(shader_program, 512, nullptr, shader_compile_log);
    std::cout << "Error. Failed to link shader program\n" << shader_compile_log << '\n';
    glDeleteProgram(shader_program);
    shader_program = 0;
    return -1;
  }
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  if (use_program) {
    glUseProgram(shader_program);
  }
  return 0;
}

GLuint create_bmp_texture(const myyuv::BMP& bmp, GLuint shader_program, const GLchar* uniform, GLuint unit) {
  assert(bmp.isValid());
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  uint8_t* data = bmp.colorData();
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp.trueWidth(), bmp.trueHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp.data);
  delete[] data;
  glGenerateMipmap(GL_TEXTURE_2D);
  if (shader_program != 0 && uniform != nullptr) {
    glUniform1i(glGetUniformLocation(shader_program, uniform), unit);
  }
  return texture;
}

static void create_plane_texture(GLuint shader_program, const GLuint unit, GLuint& tex, const uint8_t* data, const uint32_t width, const uint32_t height, const char* uniform) {
  assert(uniform);
  assert(data);
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glUniform1i(glGetUniformLocation(shader_program, uniform), unit);
}

std::vector<GLuint> create_yuv_texture(const myyuv::YUV& yuv, GLuint shader_program, const std::vector<const GLchar*>& uniforms, GLuint unit) {
  assert(yuv.isValid());
  assert(!yuv.isCompressed());
  assert(shader_program != 0);
  assert(uniforms.size() > 0);
  if (yuv.getFormatGroup() != myyuv::YUV::FormatGroup::PLANAR) {
    throw std::runtime_error("Only planar yuv group is supported");
  }
  auto planes = yuv.getYUVPlanes();
  std::vector<GLuint> texes;
  uint32_t j = 0;
  for (uint32_t i = 0; i < planes.size(); i++) {
    if (planes[i] != nullptr) {
      texes.push_back(0);
      auto width_height = yuv.getWidthHeightChannel(i);
      assert(width_height[0] != 0 && width_height[1] != 0);
      create_plane_texture(shader_program, unit + j, texes.at(j), planes[i], width_height[0], width_height[1], uniforms.at(j));
      j++;
    }
  }
  assert(texes.size() > 0);
  return texes;
}
