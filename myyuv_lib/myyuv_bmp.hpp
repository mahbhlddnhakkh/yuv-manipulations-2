#pragma once

#include <string>
#include <cstdint>

namespace myyuv {

// https://solarianprogrammer.com/2018/11/19/cpp-reading-writing-bmp-images/
#pragma pack(push, 1)
struct BMPHeader {
  // File Header
  uint8_t type[2] = { 'B', 'M' };
  uint32_t file_size = 0;
  uint16_t reserved1 = 0;
  uint16_t reserved2 = 0;
  uint32_t data_pos = 0;
  // Info Header
  uint32_t header_size = 0;
  int32_t width = 0;
  int32_t height = 0;
  uint16_t planes = 0;
  uint16_t bit_count = 0;
  uint32_t compression = 0; // assume 0
  uint32_t size_image_for_compression = 0; // assume 0
  int32_t x_pixels_per_meter = 0;
  int32_t y_pixels_per_meter = 0;
  uint32_t colors_used = 0;
  uint32_t colors_important = 0;
};

struct BMPColorHeader {
  uint32_t red_mask = 0x00ff0000;
  uint32_t green_mask = 0x0000ff00;
  uint32_t blue_mask = 0x000000ff;
  uint32_t alpha_mask = 0xff000000;
  uint32_t color_space = 0x73524742; // sRGB
  uint32_t unused[16] = { 0 };
};
#pragma pack(pop)

struct BMP {
  BMPHeader header;
  BMPColorHeader color_header;
  uint8_t* data = nullptr;
  BMP() {}
  explicit BMP(const std::string& path);
  BMP(const BMP& bmp);
  BMP& operator=(const BMP& bmp);
  BMP(BMP&& bmp) noexcept;
  BMP& operator=(BMP&& bmp) noexcept;
  ~BMP();
  inline uint32_t trueWidth() const noexcept {
    return std::abs(header.width);
  }
  inline uint32_t trueHeight() const noexcept {
    return std::abs(header.height);
  }
  uint32_t imageSize() const noexcept;
  uint8_t* colorData() const;
  uint8_t* colorDataFlipped() const;
  BMP fixedColorDataRotation() const;
  bool isValid() const noexcept;
  bool isValidHeader() const noexcept;
  void load(const std::string& path);
  void dump(const std::string& path) const;
};

} // myyuv
