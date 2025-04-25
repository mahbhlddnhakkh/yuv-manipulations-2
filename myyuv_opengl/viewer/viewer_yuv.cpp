#include "viewer.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <myyuv_opengl_shared.hpp>
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
  GLuint shader_program;
  create_shader_program(shader_program, vertex_shader_src, fragment_shader_src);
  // create textures
  auto texes = create_yuv_texture(yuv, shader_program, { "YTex", "UTex", "VTex" }, 0);
  if (texes.size() != 3) {
    throw std::runtime_error("Only 3 planes supported");
  }
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
    for (uint32_t i = 0; i < texes.size(); i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, texes[i]);
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
  for (uint32_t i = 0; i < texes.size(); i++) {
    glDeleteTextures(1, &texes[i]);
  }
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
