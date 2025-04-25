#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <myyuv.hpp>
#include <glm/glm.hpp>
#include <chrono>

extern const size_t SCREEN_WIDTH;
extern const size_t SCREEN_HEIGHT;
extern const float cube_rotation_speed;
extern const glm::mat4 projection;
extern const glm::vec3 cube_pos;
extern const glm::vec3 initial_camera_pos;

class TimerDelta {
public:
  TimerDelta() noexcept;
  GLfloat tick() noexcept;
  void reset() noexcept;
protected:
  std::chrono::system_clock::time_point time_prev;
};

struct Camera {
  glm::vec3 pos = { 0.0f, 0.0f, 0.0f };
  glm::vec3 front = { 0.0f, 0.0f, -1.0f };
  glm::vec3 up = { 0.0f, 1.0f, 0.0f };
  glm::vec3 right = { 1.0f, 0.0f, 0.0f };
  glm::vec3 worldUp = { 0.0f, 1.0f, 0.0f };
  GLfloat pitch = 0.0f;
  GLfloat yaw = -90.0f;
  GLfloat speed = 2.5f;
  GLfloat sensitivity = 2.0f;
  glm::mat4 getView() const noexcept;
  void move(int x, int y, int z, GLfloat delta);
  void turn(int x, int y);
  void update();
};

void normalize_angle(float& angle);

void create_cube(GLuint& VAO, GLuint& VBO, GLuint& EBO);

void create_parallelepiped(uint32_t width, uint32_t height, GLuint& VAO, GLuint& VBO, GLuint& EBO);

void handle_events(GLFWwindow* window, Camera& camera, GLfloat delta);

int main_bmp(const myyuv::BMP& bmp, bool force_cube, bool flip_w_h);

int main_yuv(myyuv::YUV yuv, bool force_cube, bool flip_w_h);
