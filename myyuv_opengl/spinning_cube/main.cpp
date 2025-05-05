#include "spinning_cube.hpp"

#include <myyuv_opengl_shared.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>

static void print_usage() {
  std::cout << "A spinning cube (or parallelepiped) with BMP and YUV image as a texture. Move around with WASD, look around with arrow keys, fly up with SPACE and fly down with SHIFT. The camera is an airplane-like. Press ESCAPE to exit.\n"
  << "Usage:\n"
  << "myyuv_opengl_spinning_cube /path/to/image.myyuv [params]\n"
  << "Params:\n"
  << "`-shapes n` - creates `n` shapes, where `n` is a number between 1 and " << shapes_count_max << '\n'
  << "`-force_cube` - forces shape with texture into a cube even if the image width and height are not equal\n"
  << "`-flip_width_height` - flips width and height of a texture. This will affect only the shape. Does nothing if the shape is cube\n";
  std::cout << "\nFor example:\n"
  << "myyuv_opengl_spinning_cube /path/to/image.myyuv -force_cube -shapes 10\n";
}

static void parse_args(const std::vector<std::string>& args, bool& force_cube, bool& flip_w_h, size_t& shapes_count) {
  size_t argi = 2;
  bool shapes_count_changed = false;
  for (; argi < args.size(); argi++) {
    if (args[argi] == "-shapes") {
      if (shapes_count_changed) {
        throw std::runtime_error("Too many " + args[argi] + " parameters");
      }
      shapes_count_changed = true;
      argi++;
      if (argi >= args.size()) {
        throw std::runtime_error("Shapes count argument is required");
      }
      int tmp;
      try {
        tmp = std::stoi(args[argi]);
      } catch(...) {
        throw std::runtime_error("Invalid shapes count");
      }
      if (tmp < 1 || tmp > shapes_count_max) {
        throw std::runtime_error(std::string("Shapes count must be between 1 and ") + std::to_string(shapes_count_max));
      }
      shapes_count = tmp;
    } else if (args[argi] == "-force_cube") {
      if (force_cube) {
        throw std::runtime_error("Too many " + args[argi] + " parameters");
      }
      force_cube = true;
    } else if (args[argi] == "-flip_width_height") {
      if (flip_w_h) {
        throw std::runtime_error("Too many " + args[argi] + " parameters");
      }
      flip_w_h = true;
    } else {
      throw std::runtime_error("Unknown parameter: " + args[argi]);
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc <= 1 || argc > 6) {
    print_usage();
    return 0;
  }
  int ret = 0;
  bool force_cube = false;
  bool flip_w_h = false;
  size_t shapes_count = 1;
  std::vector<std::string> args(argv, argv + argc);
  try {
    parse_args(args, force_cube, flip_w_h, shapes_count);
  } catch(...) {
    print_usage();
    throw;
  }
  const std::string path = args[1];
  IMAGE_FORMAT format = figure_out_format_magic(path);
  switch(format) {
    case IMAGE_FORMAT::BMP:
      ret = main_bmp(myyuv::BMP(path), force_cube, flip_w_h, shapes_count);
      break;
    case IMAGE_FORMAT::YUV:
      ret = main_yuv(myyuv::YUV(path), force_cube, flip_w_h, shapes_count);
      break;
    default:
      throw std::runtime_error("Unknown image format (magic) " + path);
  }
  return ret;
}
