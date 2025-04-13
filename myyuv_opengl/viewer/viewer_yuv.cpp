#include "viewer.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <thread>

static const char* vertex_shader_src = {
#include "vert.glsl"
};

static const char* fragment_shader_src = {
#include "frag_yuv.glsl"
};

static void createPlaneTexture(GLuint shader_program, const GLuint unit, GLuint& tex, const uint8_t* data, const uint32_t width, const uint32_t height, const char* uniform) {
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
  glUniform1i(glGetUniformLocation(shader_program, uniform), unit);
  glGenerateMipmap(GL_TEXTURE_2D);
}

int main_yuv(myyuv::YUV yuv) {
  if (!yuv.isValid()) {
    throw std::runtime_error("Invalid yuv");
  }
  if (yuv.isCompressed()) {
    yuv = yuv.decompress();
  }
  // init glfw
  glfwSetErrorCallback(glfw_error_callback);
  glfwInit();
  //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // create window
  GLFWwindow* window = glfwCreateWindow(yuv.header.width, yuv.header.height, "YUV OpenGL Viewer", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
  // load glew
  if (GLenum err = glewInit() != GLEW_OK) {
    std::cout << "Failed to initialize glew\n" << glewGetErrorString(err) << '\n';
    return -1;
  }
  // load shaders
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
  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  glGetShaderiv(shader_program, GL_LINK_STATUS, &shader_compile_success);
  if (!shader_compile_success) {
    glGetShaderInfoLog(shader_program, 512, nullptr, shader_compile_log);
    std::cout << "Error. Failed to link shader program\n" << shader_compile_log << '\n';
    return -1;
  }
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  glUseProgram(shader_program);
  // create textures
  GLuint y_tex, u_tex, v_tex;
  GLuint* texes[3] = { &y_tex, &u_tex, &v_tex };
  auto yuv_pointers = yuv.getYUVPlanes();
  assert(yuv_pointers[0] != nullptr && yuv_pointers[1] != nullptr && yuv_pointers[2] != nullptr);
  uint32_t width_height[3][2];
  for (uint8_t i = 0; i < 3; i++) {
    auto tmp = yuv.getWidthHeightChannel(i);
    width_height[i][0] = tmp[0];
    width_height[i][1] = tmp[1];
  }
  const char* uniforms[3] = { "YTex", "UTex", "VTex" };
  for (uint32_t i = 0; i < 3; i++) {
    createPlaneTexture(shader_program, i, *texes[i], yuv_pointers[i], width_height[i][0], width_height[i][1], uniforms[i]);
  }
  // create VAO
  const GLfloat vertices[] = {
    1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
    -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
  };
  GLuint indices[] = {
    0, 1, 3,
    1, 2, 3,
  };
  GLuint VAO;
  GLuint VBO;
  GLuint EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  // main loop
  while (!glfwWindowShouldClose(window)) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      glfwSetWindowShouldClose(window, true);
    }
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    for (uint32_t i = 0; i < 3; i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, *texes[i]);
    }
    glUseProgram(shader_program);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glfwSwapBuffers(window);
    glfwPollEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
  }
  // cleanup
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glDeleteBuffers(1, &EBO);
  glDeleteProgram(shader_program);
  glfwTerminate();
  return 0;
}
