#include "viewer.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <myyuv_opengl_shared.hpp>
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
  GLuint shader_program;
  int shader_program_res = create_shader_program(shader_program, vertex_shader_src, fragment_shader_src);
  if (shader_program_res != 0) {
    // error happened
    return shader_program_res;
  }
  // create texture
  GLuint texture = create_bmp_texture(bmp);
  // create rect
  GLuint VAO, VBO, EBO;
  create_rectangle(VAO, VBO, EBO);
  // main loop
  while (true) {
    handle_events(window);
    if (glfwWindowShouldClose(window)) {
      break;
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
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
