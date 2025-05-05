#include "spinning_cube.hpp"

#include <myyuv_opengl_shared.hpp>
#include <stdexcept>
#include <type_traits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cassert>
#include <algorithm>
#include <cstdlib>
#include <cmath>

// init constants
//
const size_t shapes_count_max = 1000;
const size_t SCREEN_WIDTH = 1000;
const size_t SCREEN_HEIGHT = 800;
const float cube_rotation_speed = 15.0f;
const glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<GLfloat>(SCREEN_WIDTH) / static_cast<GLfloat>(SCREEN_HEIGHT), 0.1f, 150.0f);

// TimeDelta
//
TimerDelta::TimerDelta() noexcept {
  reset();
}

GLfloat TimerDelta::tick() noexcept {
  auto time_current = std::chrono::high_resolution_clock::now();
  GLfloat delta = std::chrono::duration_cast<std::chrono::duration<GLfloat>>(time_current - time_prev).count();
  time_prev = time_current;
  return delta;
}

void TimerDelta::reset() noexcept {
  time_prev = std::chrono::high_resolution_clock::now();
}

// Camera
//
template <typename T>
static GLfloat sgn(T val) noexcept(noexcept(std::is_fundamental<T>::value)) {
  static_assert(std::is_fundamental<T>::value, "Type must be fundamental");
  return static_cast<GLfloat>(T(0) < val) - (val < T(0));
}

glm::mat4 Camera::getView() const noexcept {
  return glm::lookAt(pos, pos + front, up);
}

void Camera::move(int x, int y, int z, GLfloat delta) {
  glm::vec3 direction = glm::vec3(sgn(x), sgn(y), sgn(z));
  GLfloat vel = speed * delta;
  pos += front * direction[0] * vel;
  pos += right * direction[2] * vel;
  pos += up * direction[1] * vel;
}

void Camera::turn(int x, int y, GLfloat delta) {
  GLfloat x_offset = sgn(x) * sensitivity * delta * 10.0f;
  GLfloat y_offset = sgn(y) * sensitivity * delta * 10.0f;
  yaw += x_offset;
  pitch += y_offset;
  pitch = std::clamp(pitch, -89.9f, 89.9f);
  normalize_angle(yaw);
}

void Camera::update() {
  front[0] = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
  front[1] = glm::sin(glm::radians(pitch));
  front[2] = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
  front = glm::normalize(front);
  right = glm::normalize(glm::cross(front, worldUp));
  up = glm::normalize(glm::cross(right, front));
}

// geometry functions
//
void normalize_angle(float& angle) {
  if (angle > 180.0f) {
    angle -= 360.0f;
  } else if (angle < -180.0f) {
    angle += 360.0f;
  }
}

void create_cube(GLuint& VAO, GLuint& VBO, GLuint& EBO) {
  const GLfloat vertices[] = {
    // front
    1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    1.0f, -1.0f, 1.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
    -1.0f, 1.0f, 1.0f, 0.0f, 1.0f,

    // back
    1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
    1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
    -1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,

    // right
    1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
    1.0f, -1.0f, -1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 1.0f, 0.0f, 1.0f,

    // left
    -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
    -1.0f, -1.0f, 1.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
    -1.0f, 1.0f, -1.0f, 0.0f, 1.0f,

    // bottom
    // 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, // remove
    1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
    // -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, // remove

    // top
    1.0f, -1.0f, -1.0f, 1.0f, 1.0f,
    // 1.0f, -1.0f, 1.0f, 1.0f, 0.0f, // remove
    // -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, // remove
    -1.0f, -1.0f, -1.0f, 0.0f, 1.0f,
  };
  const GLuint indices[36] = {
    0, 1, 3,
    1, 2, 3,

    4, 5, 7,
    5, 6, 7,

    8, 9, 11,
    9, 10, 11,

    12, 13, 15,
    13, 14, 15,

    8, 16, 15,
    16, 15, 17,

    18, 1, 19,
    1, 2, 19
  };
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
}

void create_parallelepiped(uint32_t width, uint32_t height, GLuint& VAO, GLuint& VBO, GLuint& EBO) {
  assert(width > 0 && height > 0);
  glm::vec3 coords = glm::normalize(glm::vec3(width, height, width));
  assert(coords[0] <= 1.0f && coords[1] <= 1.0f && coords[2] <= 1.0f);
  const GLfloat vertices[] = {
    // front
    coords[0], coords[1], coords[2], 1.0f, 1.0f,
    coords[0], -coords[1], coords[2], 1.0f, 0.0f,
    -coords[0], -coords[1], coords[2], 0.0f, 0.0f,
    -coords[0], coords[1], coords[2], 0.0f, 1.0f,

    // back
    coords[0], coords[1], -coords[2], 0.0f, 1.0f,
    coords[0], -coords[1], -coords[2], 0.0f, 0.0f,
    -coords[0], -coords[1], -coords[2], 1.0f, 0.0f,
    -coords[0], coords[1], -coords[2], 1.0f, 1.0f,

    // right
    coords[0], coords[1], -coords[2], 1.0f, 1.0f,
    coords[0], -coords[1], -coords[2], 1.0f, 0.0f,
    coords[0], -coords[1], coords[2], 0.0f, 0.0f,
    coords[0], coords[1], coords[2], 0.0f, 1.0f,

    // left
    -coords[0], coords[1], coords[2], 1.0f, 1.0f,
    -coords[0], -coords[1], coords[2], 1.0f, 0.0f,
    -coords[0], -coords[1], -coords[2], 0.0f, 0.0f,
    -coords[0], coords[1], -coords[2], 0.0f, 1.0f,

    // bottom
    coords[0], coords[1], coords[2], 1.0f, 0.0f,
    -coords[0], coords[1], coords[2], 0.0f, 0.0f,

    // top
    coords[0], -coords[1], -coords[2], 1.0f, 1.0f,
    -coords[0], -coords[1], -coords[2], 0.0f, 1.0f,
  };
  const GLuint indices[36] = {
    0, 1, 3,
    1, 2, 3,

    4, 5, 7,
    5, 6, 7,

    8, 9, 11,
    9, 10, 11,

    12, 13, 15,
    13, 14, 15,

    8, 16, 15,
    16, 15, 17,

    18, 1, 19,
    1, 2, 19
  };
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
}

// GLFW events
//
static inline bool isKeyPressed(GLFWwindow* window, int key) {
  return glfwGetKey(window, key) == GLFW_PRESS;
}

void handle_events(GLFWwindow* window, Camera& camera, GLfloat delta) {
  if (isKeyPressed(window, GLFW_KEY_ESCAPE)) {
    glfwSetWindowShouldClose(window, true);
  }
  int x = 0, y = 0, z = 0;
  int view_x = 0, view_y = 0;
  // movement
  if (isKeyPressed(window, GLFW_KEY_W)) {
    x++;
  }
  if (isKeyPressed(window, GLFW_KEY_S)) {
    x--;
  }
  if (isKeyPressed(window, GLFW_KEY_A)) {
    z--;
  }
  if (isKeyPressed(window, GLFW_KEY_D)) {
    z++;
  }
  if (isKeyPressed(window, GLFW_KEY_SPACE)) {
    y++;
  }
  if (isKeyPressed(window, GLFW_KEY_LEFT_SHIFT)) {
    y--;
  }
  // view
  if (isKeyPressed(window, GLFW_KEY_RIGHT)) {
    view_x++;
  }
  if (isKeyPressed(window, GLFW_KEY_LEFT)) {
    view_x--;
  }
  if (isKeyPressed(window, GLFW_KEY_UP)) {
    view_y++;
  }
  if (isKeyPressed(window, GLFW_KEY_DOWN)) {
    view_y--;
  }
  // apply to camera
  camera.turn(view_x, view_y, delta);
  camera.move(x, y, z, delta);
  camera.update();
}

double get_sphere_generation_radius(size_t shapes_count) {
  // Because it works
  return std::sqrt(static_cast<double>(shapes_count));
}

static inline GLfloat generate_rand(double min, double max) {
  assert(min <= max);
  constexpr double rand_max_more = static_cast<double>(RAND_MAX) + 1.0;
  return (static_cast<double>(std::rand()) / (rand_max_more) * (max - min + 1.0) + min);
}

CubeData generate_random_cube_pos(const std::vector<CubeData>& cubes, double radius) {
  CubeData res;
  size_t i = 0;
  constexpr size_t i_max = 1000;
  const float two_radius = std::sqrt(3.0f) * 2.0f;
  while(i < i_max) {
    glm::vec3 tmp = { generate_rand(-radius, radius), generate_rand(-radius, radius), generate_rand(-radius, radius) };
    bool is_good = true;
    for (const auto& cube : cubes) {
      if (glm::distance(tmp, cube.pos) <= two_radius) {
        is_good = false;
        break;
      }
    }
    if (is_good) {
      res.pos = tmp;
      break;
    }
    i++;
  }
  if (i == i_max) {
    throw std::runtime_error("Unable to generate new position");
  }
  return res;
}
