#include "myyuv_bmp.hpp"
#include "viewer.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>

int main(int argc, char* argv[]) {
  if (argc <= 1 || argc > 2) {
    std::cout << "Usage: " << "/path/to/image.myyuv\n";
    return 0;
  }
  myyuv::BMPHeader bmp_header;
  myyuv::YUVHeader yuv_header;
  const uint32_t magic_size = std::max(sizeof(bmp_header.type), sizeof(yuv_header.type));
  uint8_t magic[magic_size];
  const std::string path = argv[1];
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Error opening file to read " + path);
  }
  f.read(reinterpret_cast<char*>(magic), sizeof(magic));
  f.close();
  int ret;
  if (std::equal(bmp_header.type, bmp_header.type + sizeof(bmp_header.type), magic)) {
    ret = main_bmp(myyuv::BMP(path));
  } else if (std::equal(yuv_header.type, yuv_header.type + sizeof(yuv_header.type), magic)) {
    ret = main_yuv(myyuv::YUV(path));
  } else {
    throw std::runtime_error("Unknown image format (magic) " + path);
  }
  return ret;
}
