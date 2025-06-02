#pragma once

#include "myyuv_bmp.hpp"

#include <string>
#include <unordered_map>
#include <functional>
#include <array>
#include <cstdint>

namespace myyuv {

#pragma pack(push, 1)
struct YUVHeader {
  uint8_t type[2] = { 'Y', 'U' }; // "YU"
  uint32_t fourcc_format = 0; // https://fourcc.org/yuv.php
  uint32_t data_size = 0; // file size including headers
  uint16_t compression = 0; // 0 - not compressed
  uint32_t compression_params_size = 0; // ignored if not compressed
  uint32_t compression_params_pos = 0; // ignored if not compressed
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t data_pos = 0;
  uint8_t unused[32] = { 0 }; // for whatever
};
#pragma pack(pop)

struct YUV {
  YUVHeader header;
  uint8_t* compression_params = nullptr;
  uint8_t* data = nullptr;
  enum class FormatGroup { UNKNOWN = 0, PACKED, PLANAR };
  using FourccFormat = uint32_t;
  struct FourccFormats {
    static constexpr const FourccFormat UNKNOWN = 0;
    static constexpr const FourccFormat IYUV = 0x56555949;
  };
  using Compression = uint16_t;
  struct Compressions {
    static constexpr const Compression NONE = 0;
    static constexpr const Compression DCT = 1;
  };
  static constexpr const uint32_t max_planes = 4;
  static constexpr const uint8_t no_plane = 0xff;
  static std::unordered_map<FourccFormat, std::array<uint8_t, max_planes>> yuv_order_planes_map;
  static std::unordered_map<FourccFormat, std::array<uint32_t, 2>> yuv_resolution_fraction_map;
  static std::unordered_map<FourccFormat, std::function<YUV(const BMP&)>> bmp_to_yuv_map;
  static std::unordered_map<Compression, std::unordered_map<FourccFormat, std::function<YUV(const YUV&, const void*, uint32_t)>>> compress_map;
  static std::unordered_map<Compression, std::unordered_map<FourccFormat, std::function<YUV(const YUV&)>>> decompress_map;
  static std::unordered_map<FourccFormat, FormatGroup> yuv_format_group_map;
  YUV() {}
  explicit YUV(const std::string& path);
  explicit YUV(const BMP& bmp, FourccFormat format);
  YUV(const YUV& yuv);
  YUV& operator=(const YUV& yuv);
  YUV(YUV&& yuv) noexcept;
  YUV& operator=(YUV&& yuv) noexcept;
  ~YUV();
  bool isValid() const noexcept;
  bool isValidHeader() const noexcept;
  static bool isImplementedFormat(FourccFormat format, Compression compression) noexcept;
  FourccFormat getFourccFormat() const noexcept;
  Compression getCompression() const noexcept;
  uint32_t getWidth() const noexcept;
  uint32_t getHeight() const noexcept;
  uint32_t getDataSize() const noexcept;
  std::array<uint32_t, 2> getResolutionFraction() const;
  std::array<uint32_t, 2> getWidthHeightChannel(uint8_t channel) const;
  std::array<uint32_t, max_planes> getFormatSizeBits() const;
  std::array<uint8_t, max_planes> getYUVPlanesOrder() const;
  uint32_t getImageSize() const;
  std::array<const uint8_t*, max_planes> getYUVPlanes() const;
  std::array<uint8_t*, max_planes> getYUVPlanes();
  FormatGroup getFormatGroup() const noexcept;
  static FormatGroup getFormatGroup(FourccFormat format) noexcept;
  YUV compress(Compression compression, const void* params, uint32_t params_size) const;
  YUV decompress() const;
  bool isCompressed() const noexcept;
  void load(const std::string& path);
  void load(const BMP& bmp, FourccFormat format);
  void dump(const std::string& path) const;
};

} // myyuv
