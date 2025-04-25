#include "spinning_cube.hpp"

#include <myyuv_opengl_shared.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>

static void print_usage() {
  std::cout << "Usage:\n"
  << "/path/to/image.myyuv\n"
  << "-force_cube /path/to/image.myyuv\n"
  << "-flip_width_height /path/to/image.myyuv\n";
}

int main(int argc, char* argv[]) {
  if (argc <= 1 || argc > 3) {
    print_usage();
    return 0;
  }
  int ret = 0;
  int argi = 1;
  bool force_cube = false;
  bool flip_w_h = false;
  const std::string arg1 = argv[argi];
  if (arg1 == "-force_cube") {
    force_cube = true;
    argi++;
    if (argc <= 2) {
      std::cout << "Provide path to image\n";
      print_usage();
      return 0;
    }
  } else if (arg1 == "-flip_width_height") {
    flip_w_h = true;
    argi++;
    if (argc <= 2) {
      std::cout << "Provide path to image\n";
      print_usage();
      return 0;
    }
  } else if (argc >= 3) {
    print_usage();
    return 0;
  }
  const std::string path = argv[argi];
  IMAGE_FORMAT format = figure_out_format_magic(path);
  switch(format) {
    case IMAGE_FORMAT::BMP:
      ret = main_bmp(myyuv::BMP(path), force_cube, flip_w_h);
      break;
    case IMAGE_FORMAT::YUV:
      ret = main_yuv(myyuv::YUV(path), force_cube, flip_w_h);
      break;
    default:
      throw std::runtime_error("Unknown image format (magic) " + path);
  }
  return ret;
}
