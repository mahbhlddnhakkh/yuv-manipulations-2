#include "spinning_cube.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <myyuv_opengl_shared.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

static const char* vertex_shader_src = {
#include "vert.glsl"
};

static const char* fragment_shader_src = {
#include "frag_yuv.glsl"
};

int main_yuv(myyuv::YUV yuv, bool force_cube, bool flip_w_h) {
  if (!yuv.isValid()) {
    throw std::runtime_error("Invalid yuv");
  }
  if (yuv.isCompressed()) {
    yuv = yuv.decompress();
  }
  // init glfw
  glfwSetErrorCallback(glfw_error_callback);
  glfwInit();
  GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "YUV OpenGL Spinning Cube", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
  // load glew
  if (GLenum err = glewInit() != GLEW_OK) {
    std::cout << "Failed to initialize glew\n" << glewGetErrorString(err) << '\n';
    return -1;
  }
  glEnable(GL_DEPTH_TEST);
  // load shaders
  GLuint shader_program;
  const int shader_program_res = create_shader_program(shader_program, vertex_shader_src, fragment_shader_src);
  if (shader_program_res != 0) {
    // error happened
    return shader_program_res;
  }
  // create textures
  auto texes = create_yuv_texture(yuv, shader_program, { "YTex", "UTex", "VTex" }, 0);
  if (texes.size() != 3) {
    throw std::runtime_error("Only 3 planes supported");
  }
  // create cube
  GLuint VAO, VBO, EBO;
  if (force_cube || yuv.header.width == yuv.header.height) {
    create_cube(VAO, VBO, EBO);
  } else {
    if (!flip_w_h) {
      create_parallelepiped(yuv.header.width, yuv.header.height, VAO, VBO, EBO);
    } else {
      create_parallelepiped(yuv.header.height, yuv.header.width, VAO, VBO, EBO);
    }
  }
  // setup camera
  glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, &projection[0][0]);
  Camera camera;
  camera.pos = initial_camera_pos;
  camera.update();
  // setup the rest
  float angle = 0.0f;
  TimerDelta timer_delta;
  while (true) {
    // delta time
    GLfloat delta = timer_delta.tick();
    // handle events
    handle_events(window, camera, delta);
    if (glfwWindowShouldClose(window)) {
      break;
    }
    // opengl
    glClearColor(0.7f, 0.75f, 0.71f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    for (uint32_t i = 0; i < texes.size(); i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      glBindTexture(GL_TEXTURE_2D, texes[i]);
    }
    glUseProgram(shader_program);
    glm::mat4 view = camera.getView();
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, &view[0][0]);
    glBindVertexArray(VAO);
    glm::mat4 model = glm::mat4(1.0f);
    // move the cube
    model = glm::translate(model, cube_pos);
    // make the cube spin
    angle += cube_rotation_speed * delta;
    normalize_angle(angle);
    model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, &model[0][0]);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
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
