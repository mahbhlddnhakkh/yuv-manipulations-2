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
#include "frag_bmp.glsl"
};

int main_bmp(const myyuv::BMP& bmp, bool force_cube, bool flip_w_h, size_t shapes_count) {
  assert(shapes_count >= 1 && shapes_count <= shapes_count_max);
  if (!bmp.isValid()) {
    throw std::runtime_error("Invalid bmp");
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
  // create texture
  GLuint texture = create_bmp_texture(bmp);
  // create cube
  GLuint VAO, VBO, EBO;
  if (force_cube || bmp.trueWidth() == bmp.trueHeight()) {
    create_cube(VAO, VBO, EBO);
  } else {
    if (!flip_w_h) {
      create_parallelepiped(bmp.trueWidth(), bmp.trueHeight(), VAO, VBO, EBO);
    } else {
      create_parallelepiped(bmp.trueHeight(), bmp.trueWidth(), VAO, VBO, EBO);
    }
  }
  // setup camera
  const double generation_radius = get_sphere_generation_radius(shapes_count);
  glUniformMatrix4fv(glGetUniformLocation(shader_program, "projection"), 1, GL_FALSE, &projection[0][0]);
  Camera camera;
  camera.pos = glm::vec3(generation_radius + 3.0f, 0.0f, generation_radius + 3.0f);
  camera.yaw = -135.0f;
  camera.update();
  // setup cubes
  std::vector<CubeData> cubes(shapes_count);
  for (size_t i = 1; i < shapes_count; i++) {
    cubes[i] = generate_random_cube_pos(cubes, generation_radius);
  }
  // setup the rest
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
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(shader_program);
    glm::mat4 view = camera.getView();
    glUniformMatrix4fv(glGetUniformLocation(shader_program, "view"), 1, GL_FALSE, &view[0][0]);
    glBindVertexArray(VAO);
    for (auto& cube : cubes) {
      glm::mat4 model = glm::mat4(1.0f);
      // move the cube
      model = glm::translate(model, cube.pos);
      // make the cube spin
      cube.angle += cube_rotation_speed * delta;
      normalize_angle(cube.angle);
      model = glm::rotate(model, glm::radians(cube.angle), glm::vec3(0.0f, 1.0f, 0.0f));
      glUniformMatrix4fv(glGetUniformLocation(shader_program, "model"), 1, GL_FALSE, &model[0][0]);
      glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
    glfwSwapBuffers(window);
    glfwPollEvents();
    //std::this_thread::sleep_for(std::chrono::milliseconds(40));
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
