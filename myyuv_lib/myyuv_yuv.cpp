#include "myyuv_yuv.hpp"

#include <exception>
#include <fstream>
#include <stdexcept>
#include <cassert>
#include <limits>
#include "myyuv_DCT/DCT.hpp"

namespace myyuv {

// https://stackoverflow.com/a/58568736
template <typename T>
static T divide_roundnearest(T numer, T denom) noexcept {
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

static_assert(YUV::max_planes >= 3, "max_planes must be at least 3");
static_assert(YUV::no_plane >= YUV::max_planes, "no_plane can't be less than max_planes");

// Order of planes
// Example: YUV -> 0, 1, 2 ; YVU -> 0, 2, 1
std::unordered_map<YUV::FourccFormat, std::array<uint8_t, YUV::max_planes>> YUV::yuv_order_planes_map = {
  { FourccFormats::IYUV, { 0, 1, 2, no_plane } },
};

std::unordered_map<YUV::FourccFormat, std::array<uint32_t, 2>> YUV::yuv_resolution_fraction_map = {
  { FourccFormats::IYUV, { 2, 2 } },
};

static inline void getYUV444FromRGB2x2(uint8_t yuv444[12], uint32_t i, uint32_t j, const uint8_t* rbg, uint32_t width, uint32_t pixel_bits) noexcept {
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
  { FourccFormats::IYUV, [](const BMP& bmp)->YUV {
    constexpr const FourccFormat format = FourccFormats::IYUV;
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

std::unordered_map<YUV::Compression, std::unordered_map<YUV::FourccFormat, std::function<YUV(const YUV&, const void*, uint32_t)>>> YUV::compress_map = {
  { Compressions::DCT, {
    { FourccFormats::IYUV, [](const YUV& yuv, const void* params, uint32_t params_size)->YUV {
      assert(yuv.getCompression() == Compressions::NONE);
      if (params_size != 3) {
        throw std::runtime_error("Error compression: incorrect parameters count. 3 parameters required");
      }
      std::array<uint8_t, 3> p;
      for (int i = 0; i < 3; i++) {
        p[i] = reinterpret_cast<const uint8_t*>(params)[i];
      }
      return compress_DCT_planar(yuv, p);
    }}
  }},
};

std::unordered_map<YUV::Compression, std::unordered_map<YUV::FourccFormat, std::function<YUV(const YUV&)>>> YUV::decompress_map = {
  { Compressions::DCT, {
    { FourccFormats::IYUV, [](const YUV& yuv)->YUV{
      assert(yuv.getCompression() == Compressions::DCT);
      if (yuv.header.compression_params_size != 3) {
        throw std::runtime_error("Error decompression: incorrect parameters count. 3 parameters required");
      }
      std::array<uint8_t, 3> p;
      for (int i = 0; i < 3; i++) {
        p[i] = reinterpret_cast<const uint8_t*>(yuv.compression_params)[i];
      }
      return decompress_DCT_planar(yuv, p);
    }}
  }}
};

std::unordered_map<YUV::FourccFormat, YUV::FormatGroup> YUV::yuv_format_group_map = {
  { FourccFormats::IYUV, FormatGroup::PLANAR },
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
  header.compression == static_cast<uint16_t>(Compressions::NONE) && compression_params == nullptr ||
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
  if (compression != Compressions::NONE) {
    if (!mapKeyExist(compress_map, compression) || !mapKeyExist(decompress_map, compression)) {
      return false;
    }
    return mapKeyExist(compress_map.at(compression), format) && mapKeyExist(decompress_map.at(compression), format);
  } else {
    return true;
  }
}

bool YUV::isCompressed() const noexcept {
  return getCompression() != Compressions::NONE;
}

YUV::FourccFormat YUV::getFourccFormat() const noexcept {
  return header.fourcc_format;
}

YUV::Compression YUV::getCompression() const noexcept{
  return header.compression;
}

uint32_t YUV::getWidth() const noexcept {
  return header.width;
}

uint32_t YUV::getHeight() const noexcept {
  return header.height;
}

uint32_t YUV::getDataSize() const noexcept {
  return header.data_size;
}

std::array<uint32_t, 2> YUV::getResolutionFraction() const {
  if (!isImplementedFormat(getFourccFormat(), Compressions::NONE)) {
    throw std::runtime_error("Error. Unimplemented format.");
  }
  return yuv_resolution_fraction_map.at(getFourccFormat());
}

std::array<uint32_t, 2> YUV::getWidthHeightChannel(uint8_t channel) const {
  auto order = getYUVPlanesOrder();
  if (order[channel] != no_plane) {
    switch(channel) {
      case 1:
      case 2:
        {
          auto fractions = getResolutionFraction();
          return { header.width / fractions[0], header.height / fractions[1] };
        }
      default:
        return { header.width, header.height };
    }
  } else {
    return { 0, 0 };
  }
}

std::array<uint32_t, YUV::max_planes> YUV::getFormatSizeBits() const {
  // TODO: better name? it's not bits per pixel
  if (!isImplementedFormat(getFourccFormat(), Compressions::NONE)) {
    throw std::runtime_error("Error. Unimplemented format.");
  }
  auto fractions = getResolutionFraction();
  auto order = getYUVPlanesOrder();
  uint32_t fraction = fractions[0] * fractions[1];
  std::array<uint32_t, max_planes> bits = { 8, 8 / fraction, 8 / fraction, 8 };
  for (uint32_t i = 0; i < max_planes; i++) {
    if (order[i] == no_plane) {
      bits[i] = 0;
    }
  }
  return bits;
}

std::array<uint8_t, YUV::max_planes> YUV::getYUVPlanesOrder() const {
  if (!isImplementedFormat(getFourccFormat(), Compressions::NONE)) {
    throw std::runtime_error("Error. Unimplemented format.");
  }
  if (!mapKeyExist(yuv_order_planes_map, getFourccFormat())) {
    throw std::runtime_error("Error. Planar type unimplemented (?)");
  }
  auto order = yuv_order_planes_map.at(getFourccFormat());
  assert(order[0] >= 0 && order[0] <= max_planes - 1); // luma is required
  assert([this](const auto& order)->bool{
    for (uint32_t i = 1; i < max_planes; i++) {
      if (!(order[i] >= 0 && order[i] <= max_planes - 1 || order[i] == no_plane)) {
        return false;
      }
    }
    return true;
  }(order));
  for (uint32_t i = 1; i < max_planes; i++) {
    assert(order[i] >= 0 && order[i] <= max_planes - 1 || order[i] == no_plane);
  }
  for (uint32_t i = 0; i < max_planes; i++) {
    if (order[i] != no_plane) {
      for (uint32_t j = i + 1; j < max_planes; j++) {
        assert(order[i] != order[j]);
      }
    }
  }
  return order;
}

uint32_t YUV::getImageSize() const {
  auto bits = getFormatSizeBits();
  uint32_t sz = 0;
  for (uint32_t i = 0; i < 3; i++) {
    sz += header.width * header.height * bits[i] / 8;
  }
  return sz;
}

std::array<const uint8_t*, YUV::max_planes> YUV::getYUVPlanes() const {
  auto bits = getFormatSizeBits();
  if (getFormatGroup() != FormatGroup::PLANAR) {
    throw std::runtime_error("Error. Can't get planes on non-planar type");
  }
  if (!mapKeyExist(yuv_order_planes_map, getFourccFormat())) {
    throw std::runtime_error("Error. Planar type unimplemented (?)");
  }
  auto order = getYUVPlanesOrder();
  std::array<const uint8_t*, max_planes> res = { 0 };
  assert(order[0] != no_plane);
  res[order[0]] = data;
  uint8_t o_offset = 1;
  for (uint8_t i = 1; i < max_planes; i++) {
    const uint8_t o = order[i];
    const uint8_t o_prev = order[i - o_offset];
    if (o == no_plane) {
      o_offset++;
      continue;
    }
    res[o] = res[o_prev] + header.width * header.height * bits[o_prev] / 8;
  }
  for (uint32_t i = 0; i < max_planes; i++) {
    const uint32_t o = order[i];
    if (o != no_plane && bits[o] == 0) {
      res[o] = nullptr;
    }
  }
  return res;
}

static constexpr std::array<uint8_t*, YUV::max_planes> planes_const_cast(const std::array<const uint8_t*, YUV::max_planes>& arr) {
  std::array<uint8_t*, YUV::max_planes> res{};
  for (uint8_t i = 0; i < YUV::max_planes; i++) {
    res[i] = const_cast<uint8_t*>(arr[i]);
  }
  return res;
}

std::array<uint8_t*, YUV::max_planes> YUV::getYUVPlanes() {
  return planes_const_cast(reinterpret_cast<const YUV*>(this)->getYUVPlanes());
}

YUV::FormatGroup YUV::getFormatGroup() const noexcept {
  return getFormatGroup(getFourccFormat());
}

YUV::FormatGroup YUV::getFormatGroup(FourccFormat format) noexcept {
  if (mapKeyExist(yuv_format_group_map, format)) {
    return yuv_format_group_map.at(format);
  } else {
    return FormatGroup::UNKNOWN;
  }
}

YUV YUV::compress(Compression compression, const void* params, uint32_t params_size) const {
  if (getCompression() != Compressions::NONE) {
    throw std::runtime_error("Error already compressed");
  }
  if (!mapKeyExist(compress_map, compression)) {
    throw std::runtime_error("Error this compression is unimplemented");
  }
  const auto& comp = compress_map.at(compression);
  FourccFormat format = getFourccFormat();
  if (!mapKeyExist(comp, format)) {
    throw std::runtime_error("Error compression for this format is unimplemented");
  }
  return comp.at(format)(*this, params, params_size);
}

YUV YUV::decompress() const {
  Compression compression = getCompression();
  if (compression == Compressions::NONE) {
    return *this;
  }
  if (!mapKeyExist(decompress_map, compression)) {
    throw std::runtime_error("Error this decompression is unimplemented");
  }
  const auto& comp = decompress_map.at(compression);
  FourccFormat format = getFourccFormat();
  if (!mapKeyExist(comp, format)) {
    throw std::runtime_error("Error decompression for this format is unimplemented");
  }
  return comp.at(format)(*this);
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
  f.seekg(res.header.data_pos, f.beg);
  res.header.compression_params_pos = sizeof(res.header);
  res.header.data_pos = res.header.compression_params_pos + res.header.compression_params_size;
  if (res.getCompression() == Compressions::NONE) {
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
