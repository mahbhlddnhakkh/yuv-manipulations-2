#include "viewer.hpp"

#include <myyuv_opengl_shared.hpp>
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
  if (argc <= 1 || argc > 2) {
    std::cout << "A BMP and YUV image viewer with OpenGL as a backend. Press ESCAPE to exit.\n"
    << "Usage:\n"
    << "myyuv_opengl_viewer /path/to/image.myyuv\n";
    return 0;
  }
  int ret = 0;
  const std::string path = argv[1];
  IMAGE_FORMAT format = figure_out_format_magic(path);
  switch(format) {
    case IMAGE_FORMAT::BMP:
      ret = main_bmp(myyuv::BMP(path));
      break;
    case IMAGE_FORMAT::YUV:
      ret = main_yuv(myyuv::YUV(path));
      break;
    default:
      throw std::runtime_error("Unknown image format (magic) " + path);
  }
  return ret;
}
