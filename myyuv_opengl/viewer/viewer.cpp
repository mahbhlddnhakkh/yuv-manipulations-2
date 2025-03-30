#include "viewer.hpp"
#include <iostream>

void glfw_error_callback(GLint error, const GLchar* desc) {
  std::cout << "GLFW error " << error << ": " << desc << '\n';
  exit(1);
}
