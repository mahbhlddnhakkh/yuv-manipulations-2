#include "myyuv_yuv.hpp"

#include <exception>
#include <fstream>
#include <stdexcept>
#include <cassert>
#include <limits>

namespace myyuv {

// https://stackoverflow.com/a/58568736
template <typename T>
static T divide_roundnearest(T numer, T denom) {
  static_assert(std::numeric_limits<T>::is_integer, "Only integer types are allowed");

  T result = ((numer) < 0) != ((denom) < 0) ?
    ((numer) - ((denom)/2)) / (denom) :
    ((numer) + ((denom)/2)) / (denom);
  return result;
}

// https://stackoverflow.com/a/36835959
static inline constexpr unsigned char operator "" _uchar( unsigned long long arg ) noexcept {
  return static_cast< unsigned char >( arg );
}

// Order of planes
// Example: YUV -> 0, 1, 2 ; YVU -> 0, 2, 1
std::unordered_map<YUV::FourccFormat, std::array<uint8_t, 3>> YUV::yuv_order_planes_map = {
  { FourccFormat::IYUV, { 0, 1, 2 } },
};

std::unordered_map<YUV::FourccFormat, std::array<uint32_t, 2>> YUV::yuv_resolution_fraction_map = {
  { FourccFormat::IYUV, { 2, 2 } },
};

static inline void getYUV444FromRGB2x2(uint8_t yuv444[12], uint32_t i, uint32_t j, const uint8_t* rbg, uint32_t width, uint32_t pixel_bits) {
  const uint32_t pixel_bytes = pixel_bits / 8;
  const uint32_t loc = i + j * width;
  const uint32_t loc_down = loc + width;
  const uint32_t loc_ayuv = loc * pixel_bytes;
  const uint32_t loc_down_ayuv = loc_down * pixel_bytes;
  const uint32_t locs[4] = { loc_ayuv, loc_ayuv + pixel_bytes, loc_down_ayuv, loc_down_ayuv + pixel_bytes };
  uint32_t jj = 0;
  for (uint32_t ii = 0; ii < 12; ii += 3) {
    const float B = static_cast<float>(rbg[locs[jj]]);
    const float G = static_cast<float>(rbg[locs[jj] + 1]);
    const float R = static_cast<float>(rbg[locs[jj] + 2]);
    const float Y = 0.299f * R + 0.587f * G + 0.114f * B;
    yuv444[ii] = static_cast<uint8_t>(Y); // Y
    yuv444[ii + 1] = static_cast<uint8_t>((B - Y) * 0.564f) + 128; // Cb
    yuv444[ii + 2] = static_cast<uint8_t>((R - Y) * 0.713f) + 128; // Cr
    jj++;
  }
}

std::unordered_map<YUV::FourccFormat, std::function<YUV(const BMP&)>> YUV::bmp_to_yuv_map = {
  { FourccFormat::IYUV, [](const BMP& bmp)->YUV {
    constexpr const FourccFormat format = FourccFormat::IYUV;
    assert(bmp.isValid());
    assert(bmp.header.bit_count == 32); // TODO: test 24
    YUV res;
    res.header.fourcc_format = static_cast<uint32_t>(format);
    //std::array<uint32_t, 3> data_size_bits = yuv_format_size_bits_map.at(format);
    const uint32_t width = bmp.trueWidth();
    const uint32_t height = bmp.trueHeight();
    assert(width % 2 == 0 && height % 2 == 0);
    uint8_t* data = bmp.colorData();
    res.header.width = width;
    res.header.height = height;
    res.header.data_size = width * height * 3 / 2; // width * height + width * height / 2
    res.header.data_pos = sizeof(YUVHeader);
    res.data = new uint8_t[res.header.data_size];
    uint8_t* y = res.data;
    uint8_t* u = &(res.data[width * height]);
    uint8_t* v = &(res.data[width * height * 5 / 4]);
    for (uint32_t j = 0; j < height; j += 2) {
      for (uint32_t i = 0; i < width; i += 2) {
        const uint32_t loc = i + j * width;
        const uint32_t loc_down = loc + width;
        uint8_t yuv444[12];
        getYUV444FromRGB2x2(yuv444, i, j, data, width, bmp.header.bit_count);
        const uint8_t Cb = divide_roundnearest(yuv444[1], 4_uchar) + divide_roundnearest(yuv444[4], 4_uchar) + divide_roundnearest(yuv444[7], 4_uchar) + divide_roundnearest(yuv444[10], 4_uchar);
        const uint8_t Cr = divide_roundnearest(yuv444[2], 4_uchar) + divide_roundnearest(yuv444[5], 4_uchar) + divide_roundnearest(yuv444[8], 4_uchar) + divide_roundnearest(yuv444[11], 4_uchar);
        y[loc] = yuv444[0];
        y[loc + 1] = yuv444[3];
        y[loc_down] = yuv444[6];
        y[loc_down + 1] = yuv444[9];
        const uint32_t k = (i + j * width / 2) / 2;
        u[k] = Cb;
        v[k] = Cr;
      }
    }
    delete[] data;
    return res;
  }},
};

std::unordered_map<YUV::Compression, std::unordered_map<YUV::FourccFormat, std::function<YUV(const YUV&, void*, uint32_t)>>> YUV::compress_map = {
// TODO:
};

std::unordered_map<YUV::Compression, std::unordered_map<YUV::FourccFormat, std::function<YUV(const YUV&, void*, uint32_t)>>> YUV::decompress_map = {
// TODO:
};

YUV::YUV(const std::string& path) : YUV() {
  load(path);
}

YUV::YUV(const BMP& bmp, FourccFormat format) : YUV() {
  load(bmp, format);
}

YUV::YUV(const YUV& yuv) {
  operator=(yuv);
}

YUV& YUV::operator=(const YUV& yuv) {
  assert(yuv.isValidHeader());
  auto copy_data_lambda = [](uint8_t* data, uint8_t*& new_data, const uint8_t* tmp_data, uint32_t data_size, uint32_t tmp_data_size) {
    if (tmp_data != nullptr) {
      if (data == nullptr || tmp_data_size > data_size) {
        new_data = new uint8_t[tmp_data_size];
      } else {
        new_data = data;
      }
      std::copy(tmp_data, tmp_data + tmp_data_size, new_data);
    }
  };
  uint8_t* new_data = nullptr;
  uint8_t* new_compression_params = nullptr;
  try {
    copy_data_lambda(data, new_data, yuv.data, header.data_size, yuv.header.data_size);
    copy_data_lambda(compression_params, new_compression_params, yuv.compression_params, header.compression_params_size, yuv.header.compression_params_size);
  } catch (...) {
    if (new_data != data) {
      delete[] new_data;
    }
    if (new_compression_params != compression_params) {
      delete[] new_compression_params;
    }
    throw;
  }
  if (new_data != data || new_data == nullptr) {
    delete[] data;
    data = new_data;
  }
  if (new_compression_params != compression_params || new_compression_params == nullptr) {
    delete[] compression_params;
    compression_params = new_compression_params;
  }
  header = yuv.header;
  return *this;
}

YUV::YUV(YUV&& yuv) noexcept {
  operator=(std::move(yuv));
}

YUV& YUV::operator=(YUV&& yuv) noexcept {
  std::swap(header, yuv.header);
  std::swap(compression_params, yuv.compression_params);
  std::swap(data, yuv.data);
  return *this;
}

YUV::~YUV() {
  delete[] data;
  delete[] compression_params;
}

bool YUV::isValid() const noexcept {
  return data != nullptr &&
  (header.compression_params_size > 0 && compression_params != nullptr ||
  header.compression == static_cast<uint16_t>(Compression::NONE) && compression_params == nullptr ||
  header.compression_params_size == 0 && compression_params == nullptr) &&
  isValidHeader();
}

bool YUV::isValidHeader() const noexcept {
  return header.type[0] == 'Y' && header.type[1] == 'U' &&
  isImplementedFormat(getFourccFormat(), getCompression()) &&
  header.width > 0 && header.height > 0 &&
  header.data_pos >= sizeof(YUVHeader) + header.compression_params_size &&
  header.data_size > 0;
}

template<typename T, typename U>
inline static bool mapKeyExist(const std::unordered_map<T, U>& map, const T& key) noexcept {
  return map.find(key) != map.end();
}

bool YUV::isImplementedFormat(FourccFormat format, Compression compression) noexcept {
  if (!mapKeyExist(bmp_to_yuv_map, format) || !mapKeyExist(yuv_resolution_fraction_map, format)) {
    return false;
  }
  if (compression != Compression::NONE) {
    if (!mapKeyExist(compress_map, compression) || !mapKeyExist(decompress_map, compression)) {
      return false;
    }
    return mapKeyExist(compress_map.at(compression), format) && mapKeyExist(decompress_map.at(compression), format);
  } else {
    return true;
  }
}

bool YUV::isCompressed() const noexcept {
  return getCompression() != Compression::NONE;
}

std::array<uint32_t, 2> YUV::getResolutionFraction() const {
  if (!isImplementedFormat(getFourccFormat(), Compression::NONE)) {
    throw std::runtime_error("Error. Unimplemented format.");
  }
  return yuv_resolution_fraction_map.at(getFourccFormat());
}

std::array<uint32_t, 3> YUV::getFormatSizeBits() const {
  if (!isImplementedFormat(getFourccFormat(), Compression::NONE)) {
    throw std::runtime_error("Error. Unimplemented format.");
  }
  auto fractions = getResolutionFraction();
  uint32_t fraction = fractions[0] * fractions[1];
  return { 8, 8 / fraction, 8 / fraction };
}

uint32_t YUV::getImageSize() const {
  auto bits = getFormatSizeBits();
  uint32_t sz = 0;
  for (uint32_t i = 0; i < 3; i++) {
    sz += header.width * header.height * bits[i] / 8;
  }
  return sz;
}

std::array<uint8_t*, 3> YUV::getYUVPlanes() const {
  auto bits = getFormatSizeBits();
  if (getFormatGroup() != FormatGroup::PLANAR) {
    throw std::runtime_error("Error. Can't get planes on non-planar type");
  }
  if (!mapKeyExist(yuv_order_planes_map, getFourccFormat())) {
    throw std::runtime_error("Error. Planar type unimplemented (?)");
  }
  auto order = yuv_order_planes_map.at(getFourccFormat());
  std::array<uint8_t*, 3> res;
  res[order[0]] = data;
  for (uint32_t i = 1; i < 3; i++) {
    const uint32_t o = order[i];
    assert(o >= 0 && o <= 2);
    res[o] = res[o-1] + header.width * header.height * bits[o-1] / 8;
  }
  return res;
}

YUV::FormatGroup YUV::getFormatGroup() const noexcept {
  return getFormatGroup(getFourccFormat());
}

YUV::FormatGroup YUV::getFormatGroup(FourccFormat format) noexcept {
  switch (format) {
    case FourccFormat::IYUV:
      return FormatGroup::PLANAR;
    default:
      return FormatGroup::UNKNOWN;
  }
}

void YUV::load(const std::string& path) {
  YUV res;
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Error opening file to read " + path);
  }
  f.read(reinterpret_cast<char*>(&res.header), sizeof(res.header));
  if (!res.isValidHeader()) {
    throw std::runtime_error("Error bad header " + path);
  }
  if (res.header.compression_params_size > 0) {
    f.seekg(res.header.compression_params_pos, f.beg);
    res.compression_params = new uint8_t[res.header.compression_params_size];
    f.read(reinterpret_cast<char*>(res.compression_params), res.header.compression_params_size);
  }
  f.seekg(res.header.data_pos);
  res.header.compression_params_pos = sizeof(res.header);
  res.header.data_pos = res.header.compression_params_pos + res.header.compression_params_size;
  if (res.getCompression() == Compression::NONE) {
    res.header.data_size = res.getImageSize();
  }
  res.data = new uint8_t[res.header.data_size];
  f.read(reinterpret_cast<char*>(res.data), res.header.data_size);
  assert(res.isValid());
  std::swap(*this, res);
}

void YUV::load(const BMP& bmp, FourccFormat format) {
  if (!bmp.isValid()) {
    throw std::runtime_error("BMP is invalid");
  }
  if (mapKeyExist(bmp_to_yuv_map, format)) {
    YUV tmp = bmp_to_yuv_map.at(format)(bmp);
    assert(tmp.isValid());
    std::swap(*this, tmp);
  } else {
    throw std::runtime_error("Incorrect format");
  }
}

void YUV::dump(const std::string& path) const {
  assert(isValid());
  std::ofstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Error opening file to write " + path);
  }
  f.write(reinterpret_cast<const char*>(&header), sizeof(header));
  if (compression_params != nullptr) {
    f.write(reinterpret_cast<const char*>(compression_params), header.compression_params_size);
  }
  f.write(reinterpret_cast<const char*>(data), header.data_size);
}

} // myyuv
