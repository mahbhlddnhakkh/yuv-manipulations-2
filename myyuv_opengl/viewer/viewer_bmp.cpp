#include "viewer.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

static const char* vertex_shader_src = {
#include "vert.glsl"
};

static const char* fragment_shader_src = {
#include "frag_bmp.glsl"
};

int main_bmp(const myyuv::BMP& bmp) {
  if (!bmp.isValid()) {
    throw std::runtime_error("Invalid bmp");
  }
  assert(bmp.header.width > 0 && bmp.header.height > 0);
  // init glfw
  glfwSetErrorCallback(glfw_error_callback);
  glfwInit();
  //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  // create window
  GLFWwindow* window = glfwCreateWindow(bmp.trueWidth(), bmp.trueHeight(), "YUV OpenGL Viewer", nullptr, nullptr);
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
  // create texture
  GLuint texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bmp.trueWidth(), bmp.trueHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, bmp.data);
  glGenerateMipmap(GL_TEXTURE_2D);
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
    glBindTexture(GL_TEXTURE_2D, texture);
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
  glDeleteTextures(1, &texture);
  glfwTerminate();
  return 0;
}
