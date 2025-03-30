#include <myyuv.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <cassert>

template<typename T, typename U>
inline static bool mapKeyExist(const std::unordered_map<T, U>& map, const T& key) noexcept {
  return map.find(key) != map.end();
}

static std::unordered_map<std::string, myyuv::YUV::FourccFormat> format_map = {
  { "IYUV", myyuv::YUV::FourccFormat::IYUV },
};

static void print_usage() {
  std::cout << "Usage:\n"
  << "/path/to/image -info\n"
  << "/path/to/image.bmp -to_yuv format -o /path/to/new_image.myyuv\n"
  << "/path/to/image.myyuv -compress compression [params...] -o /path/to/new_image.myyuv\n"
  << "/path/to/image.myyuv -decompress [-o /path/to/new_image.myyuv]\n";
}

static void process_bmp(const myyuv::BMP& bmp, int argi, std::vector<std::string> args) {
  if (args[argi] == "-info") {
    assert(bmp.isValid());
    std::cout
    << "Type: " << bmp.header.type[0] << bmp.header.type[1] << '\n'
    << "File size: " << bmp.header.file_size << '\n'
    << "Data size: " << bmp.header.width * bmp.header.height * bmp.header.bit_count / 8 << '\n'
    << "Width: " << bmp.header.width << '\n'
    << "Height: " << bmp.header.height << '\n'
    << "Bit count: " << bmp.header.bit_count << '\n'
    << "Valid: " << bmp.isValid() << '\n';
    return;
  } else if (args[argi] == "-to_yuv") {
    if (args.size() != argi + 4) {
      std::cout << "Invalid arguments amount. " << (argi + 4) << " is required\n";
      print_usage();
      return;
    }
    if (!mapKeyExist(format_map, args[argi + 1])) {
      throw std::runtime_error("Format is not registered: " + args[argi + 1]);
    }
    if (args[argi + 2] != "-o") {
      std::cout << (argi + 2) << " argument must be `-o` instead of " << args[argi + 2] << '\n';
      print_usage();
      return;
    }
    myyuv::YUV yuv(bmp, format_map.at(args[argi + 1]));
    yuv.dump(args[argi + 3]);
  } else {
    std::cout << "Invalid command " << args[argi] << '\n';
    print_usage();
    return;
  }
}

static void process_yuv(const myyuv::YUV& yuv, int argi, std::vector<std::string> args) {
  if (args[argi] == "-info") {
    std::cout
    << "Type: " << yuv.header.type[0] << yuv.header.type[1] << '\n'
    << "FourCC Format: 0x" << std::hex << yuv.header.fourcc_format << std::dec << '\n'
    << "File size: " << (sizeof(yuv.header) + yuv.header.compression_params_size + yuv.header.data_size) << '\n'
    << "Data size: " << yuv.header.data_size << '\n'
    << "Compression: " << yuv.header.compression << '\n'
    << "Compression params size: " << yuv.header.compression_params_size << '\n'
    << "Width: " << yuv.header.width << '\n'
    << "Height: " << yuv.header.height << '\n'
    << "Valid: " << yuv.isValid() << '\n';
    return;
  } else if (args[argi] == "-compress") {
    // TODO:
    std::cout << "Compression is not implemented yet\n";
    return;
  } else if (args[argi] == "-decompress") {
    // TODO:
    std::cout << "Decompression is not implemented yet\n";
    return;
  } else {
    std::cout << "Invalid command " << args[argi] << '\n';
    print_usage();
    return;
  }
}

static int _main(int argc, char* argv[]) {
  if (argc <= 2) {
    print_usage();
    return 0;
  }
  std::vector<std::string> args(argv, argv + argc);
  myyuv::BMPHeader bmp_header;
  myyuv::YUVHeader yuv_header;
  const uint32_t magic_size = std::max(sizeof(bmp_header.type), sizeof(yuv_header.type));
  uint8_t magic[magic_size];
  const std::string path = args[1];
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Error opening file to read " + path);
  }
  f.read(reinterpret_cast<char*>(magic), sizeof(magic));
  f.close();
  int ret = 0;
  if (std::equal(bmp_header.type, bmp_header.type + sizeof(bmp_header.type), magic)) {
    process_bmp(myyuv::BMP(path), 2, args);
  } else if (std::equal(yuv_header.type, yuv_header.type + sizeof(yuv_header.type), magic)) {
    process_yuv(myyuv::YUV(path), 2, args);
  } else {
    throw std::runtime_error("Unknown image format (magic) " + path);
  }
  return ret;
}

int main(int argc, char* argv[]) {
  try {
    return _main(argc, argv);
  } catch(...) {
    print_usage();
    throw;
  }
}
