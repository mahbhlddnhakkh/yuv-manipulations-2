#include <myyuv.hpp>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>

class MyTimer {
public:
  MyTimer() noexcept {
    auto current_time = std::chrono::high_resolution_clock::now();
    _start = current_time;
    _end = current_time;
  }
  void start() noexcept {
    _start = std::chrono::high_resolution_clock::now();
  }
  void end() noexcept {
    _end = std::chrono::high_resolution_clock::now();
  }
  float getDurationMs() const noexcept {
    return std::chrono::duration_cast<std::chrono::duration<float>>(_end - _start).count();
  }
  static float measureTimeMs(std::function<void()> f) {
    MyTimer timer;
    timer.start();
    f();
    timer.end();
    return timer.getDurationMs();
  }
protected:
  std::chrono::system_clock::time_point _start;
  std::chrono::system_clock::time_point _end;
};

inline static void printTimeMeasurement(float time_ms, const std::string& text = "") {
  std::cout << text << " : " << time_ms << " ms\n";
}

template<typename T, typename U>
inline static bool mapKeyExist(const std::unordered_map<T, U>& map, const T& key) noexcept {
  return map.find(key) != map.end();
}

static std::unordered_map<std::string, myyuv::YUV::FourccFormat> format_strings_map = {
  { "IYUV", myyuv::YUV::FourccFormat::IYUV },
};

static std::unordered_map<std::string, myyuv::YUV::Compression> compression_strings_map = {
  { "DCT", myyuv::YUV::Compression::DCT },
};

static std::unordered_map<myyuv::YUV::Compression, std::function<myyuv::YUV(const myyuv::YUV&, const std::vector<std::string>&)>> compression_map = {
  { myyuv::YUV::Compression::DCT, [](const myyuv::YUV& yuv, const std::vector<std::string>& params)->myyuv::YUV {
    if (params.size() > 3) {
      throw std::runtime_error("Error. Too many compression parameters. Can't be more than 3 parameters.");
    }
    if (params.size() == 0) {
      throw std::runtime_error("Error. Too few compression parameters. Must be at least one.");
    }
    std::vector<uint8_t> params_res(3);
    for (size_t i = 0; i < params.size(); i++) {
      int tmp = std::stoi(params[i]);
      if (tmp < 1 || tmp > 100) {
        throw std::runtime_error("Error. Compression parameters for DCT must range between [1..100].");
      }
      params_res[i] = tmp;
    }
    // fill the rest if given 1 or 2 parameters instead of 3
    for (size_t i = params.size() - 1; i < 3; i++) {
      params_res[i] = params_res[params.size() - 1];
    }
    return yuv.compress(myyuv::YUV::Compression::DCT, params_res.data(), params_res.size());
  }},
};

static void print_usage() {
  std::cout << "Usage:\n"
  << "/path/to/image -info\n"
  << "/path/to/image.bmp -to_yuv format -o /path/to/new_image.myyuv\n"
  << "/path/to/image.myyuv -compress compression [params...] -o /path/to/new_image.myyuv\n"
  << "/path/to/image.myyuv -decompress -o /path/to/new_image.myyuv\n";
  std::cout << "\nYUV formats:\n";
  for (const auto& it: format_strings_map) {
    std::cout << it.first << '\n';
  }
  std::cout << "\nCompression formats for YUV:\n";
  for (const auto& it : compression_strings_map) {
    std::cout << it.first << '\n';
  }
}

static int process_bmp(const myyuv::BMP& bmp, int argi, const std::vector<std::string>& args) {
  if (args[argi] == "-info") {
    std::cout
    << "Type: " << bmp.header.type[0] << bmp.header.type[1] << '\n'
    << "File size: " << bmp.header.file_size << '\n'
    << "Data size: " << bmp.header.width * bmp.header.height * bmp.header.bit_count / 8 << '\n'
    << "Width: " << bmp.header.width << '\n'
    << "Height: " << bmp.header.height << '\n'
    << "Bit count: " << bmp.header.bit_count << '\n'
    << "Valid: " << bmp.isValid() << '\n';
    return 0;
  } else if (args[argi] == "-to_yuv") {
    if (args.size() != argi + 4) {
      std::cout << "Invalid arguments amount. " << (argi + 4) << " is required\n";
      print_usage();
      return 1;
    }
    if (!mapKeyExist(format_strings_map, args[argi + 1])) {
      throw std::runtime_error("Format is not registered: " + args[argi + 1]);
    }
    if (args[argi + 2] != "-o") {
      std::cout << (argi + 2) << " argument must be `-o` instead of " << args[argi + 2] << '\n';
      print_usage();
      return 1;
    }
    myyuv::YUV yuv;
    printTimeMeasurement(MyTimer::measureTimeMs([&](){
      yuv = myyuv::YUV(bmp, format_strings_map.at(args[argi + 1]));
    }), "BMP to YUV (" + args[argi + 1] + ")");
    yuv.dump(args[argi + 3]);
    return 0;
  } else {
    std::cout << "Invalid command " << args[argi] << '\n';
    print_usage();
    return 1;
  }
}

static int process_yuv(const myyuv::YUV& yuv, int argi, const std::vector<std::string>& args) {
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
    return 0;
  } else if (args[argi] == "-compress") {
    argi++;
    if (argi >= args.size()) {
      std::cout << "Invalid arguments. Specify compression algorithm, compression parameters and output.\n";
      print_usage();
      return 1;
    }
    std::string compression_str = args[argi++];
    if (!mapKeyExist(compression_strings_map, compression_str)) {
      throw std::runtime_error("Compression not registered: " + compression_str);
    }
    myyuv::YUV::Compression compression = compression_strings_map.at(compression_str);
    if (!mapKeyExist(compression_map, compression)) {
      throw std::runtime_error("Compression not registered: " + compression_str);
    }
    std::vector<std::string> params;
    while (argi < args.size() && args[argi] != "-o") {
      params.push_back(args[argi++]);
    }
    argi++;
    if (argi >= args.size()) {
      std::cout << "Invalid argument, last arguments must be `-o /path/to/new_image.myyuv`\n";
      print_usage();
      return 1;
    }
    myyuv::YUV compressed_yuv;
    std::string params_as_string = " ";
    for (const auto& p : params) {
      params_as_string += p + " ";
    }
    printTimeMeasurement(MyTimer::measureTimeMs([&](){
      compressed_yuv = compression_map.at(compression)(yuv, params);
    }), "YUV DCT compression (" + params_as_string + ")");
    compressed_yuv.dump(args[argi]);
    return 0;
  } else if (args[argi] == "-decompress") {
    if (!yuv.isCompressed()) {
      std::cout << "Nothing to decompress, image is not compressed\n";
      return 1;
    }
    argi++;
    if (args.size() != argi + 2) {
      std::cout << "Invalid arguments amount. " << (argi + 2) << " is required\n";
      print_usage();
      return 1;
    }
    if (args[argi] != "-o") {
      std::cout << (argi) << " argument must be `-o` instead of " << args[argi] << '\n';
      print_usage();
      return 1;
    }
    myyuv::YUV decompressed_yuv;
    printTimeMeasurement(MyTimer::measureTimeMs([&](){
      decompressed_yuv = yuv.decompress();
    }), "YUV DCT decompression");
    decompressed_yuv.dump(args[argi + 1]);
    return 0;
  } else {
    std::cout << "Invalid command " << args[argi] << '\n';
    print_usage();
    return 1;
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
    ret = process_bmp(myyuv::BMP(path), 2, args);
  } else if (std::equal(yuv_header.type, yuv_header.type + sizeof(yuv_header.type), magic)) {
    ret = process_yuv(myyuv::YUV(path), 2, args);
  } else {
    throw std::runtime_error("Unknown image format (magic) " + path);
  }
  if (ret == 0) {
    std::cout << "Success!\n";
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
