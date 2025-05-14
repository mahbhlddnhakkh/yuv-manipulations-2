#include "myyuv_bmp.hpp"

#include <fstream>
#include <stdexcept>
#include <cassert>

namespace myyuv {

BMP::BMP(const std::string& path) : BMP() {
  load(path);
}

BMP::BMP(const BMP& bmp) : BMP() {
  operator=(bmp);
}

BMP& BMP::operator=(const BMP& bmp) {
  assert(bmp.isValidHeader());
  uint8_t* new_data = nullptr;
  const uint32_t tmp_image_size = bmp.imageSize();
  if (bmp.data != nullptr) {
    try { // just in case
      if (data == nullptr || tmp_image_size > imageSize()) { // I think it's better this way instead of `bmp.image_size != image_size`
        new_data = new uint8_t[tmp_image_size];
      } else {
        new_data = data;
      }
      std::copy(bmp.data, bmp.data + tmp_image_size, new_data);
      //if (new_data != data) {
      //  delete[] data;
      //  data = new_data;
      //}
    } catch (...) {
      if (new_data != data) {
        delete[] new_data;
      }
      throw;
    }
  }// else if (data != nullptr) {
  //  delete[] data;
  //  data = nullptr;
  //}
  if (new_data != data || new_data == nullptr) {
    delete[] data;
    data = new_data;
  }
  header = bmp.header;
  color_header = bmp.color_header;
  return *this;
}

BMP::BMP(BMP&& bmp) noexcept : BMP() {
  operator=(std::move(bmp));
}

BMP& BMP::operator=(BMP&& bmp) noexcept {
  std::swap(header, bmp.header);
  std::swap(color_header, bmp.color_header);
  std::swap(data, bmp.data);
  return *this;
}

BMP::~BMP() {
  delete[] data;
}

uint32_t BMP::imageSize() const noexcept {
  // I don't think overflow matters
  return trueWidth() * trueHeight() * header.bit_count / 8;
}

uint8_t* BMP::colorData() const {
  if (!isValid()) {
    throw std::runtime_error("BMP data is invalid");
  }
  const uint32_t size = imageSize();
  const uint32_t bytes_per_pixel = header.bit_count / 8;
  uint8_t* res = new uint8_t[size];
  if (header.width > 0 && header.height < 0) {
    std::copy(data, data + size, res);
  } else if (header.width < 0 && header.height > 0) {
    for (uint32_t i = 0; i < size; i += bytes_per_pixel) {
      for (uint32_t j = 0; j < bytes_per_pixel; j++) {
        res[i + j] = data[size - bytes_per_pixel - i + j];
      }
    }
  } else if (header.width > 0 && header.height > 0) {
    for (uint32_t i = 0; i < header.height; i++) {
      std::copy(data + bytes_per_pixel * header.width * (header.height - i - 1), data + bytes_per_pixel * header.width * (header.height - i), res + bytes_per_pixel * header.width * i);
    }
  } else {
    throw std::runtime_error("Unaccounted width and height sign");
  }
  return res;
}

uint8_t* BMP::colorDataFlipped() const {
  if (!isValid()) {
    throw std::runtime_error("BMP data is invalid");
  }
  const uint32_t size = imageSize();
  const uint32_t bytes_per_pixel = header.bit_count / 8;
  uint8_t* res = new uint8_t[size];
  if (header.width > 0 && header.height > 0) {
    std::copy(data, data + size, res);
  } else if (header.width > 0 && header.height < 0) {
    for (uint32_t i = 0; i < header.height; i++) {
      std::copy(data + bytes_per_pixel * header.width * (header.height - i - 1), data + bytes_per_pixel * header.width * (header.height - i), res + bytes_per_pixel * header.width * i);
    }
  } else {
    throw std::runtime_error("Unaccounted width and height sign");
  }
  return res;
}

BMP BMP::fixedColorDataRotation() const {
  assert(isValid());
  uint8_t* color_data = colorData();
  BMP res;
  res.header = header;
  res.color_header = color_header;
  res.data = color_data;
  res.header.width = trueWidth();
  res.header.height = -trueHeight();
  return res;
}

bool BMP::isValid() const noexcept {
  return data != nullptr && isValidHeader();
}
bool BMP::isValidHeader() const noexcept {
  return header.type[0] == 'B' && header.type[1] == 'M' &&
  //header.width > 0 && header.height > 0 && // Not dealing with negative width and height
  header.width % 4 == 0 && // not dealing with padding
  header.bit_count > 0 &&
  header.header_size > 0 &&
  (header.compression == 0 || header.compression == 3) && // Not compressing BMP
  header.colors_used == 0 && header.colors_important == 0 &&
  // Not dealing with different color masks
  color_header.red_mask == 0x00ff0000 && color_header.green_mask == 0x0000ff00 &&
  color_header.blue_mask == 0x000000ff && (color_header.alpha_mask == 0xff000000 || color_header.alpha_mask == 0) &&
  color_header.color_space == 0x73524742; // not dealing with anything other than sRGB
}

void BMP::load(const std::string& path) {
  BMP res;
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Error opening file to read " + path);
  }
  f.read(reinterpret_cast<char*>(&res.header), sizeof(res.header));
  if (res.header.bit_count == 32) {
    f.read(reinterpret_cast<char*>(&res.color_header), sizeof(res.color_header));
  }
  f.seekg(res.header.data_pos, f.beg);

  if (res.header.bit_count == 32) {
    res.header.data_pos = sizeof(res.header) + sizeof(res.color_header);
  } else {
    res.header.data_pos = sizeof(res.header);
  }
  const uint32_t res_image_size = res.imageSize();
  res.header.file_size = res.header.data_pos + res_image_size;

  if (!res.isValidHeader()) {
    throw std::runtime_error("Error bad header " + path);
  }
  res.data = new uint8_t[res_image_size];
  f.read(reinterpret_cast<char*>(res.data), res_image_size);
  assert(res.isValid());
  std::swap(*this, res);
}

void BMP::dump(const std::string& path) const {
  assert(isValid());
  std::ofstream f(path, std::ios::binary);
  if (!f) {
    throw std::runtime_error("Error opening file to write " + path);
  }
  f.write(reinterpret_cast<const char*>(&header), sizeof(header));
  if (header.bit_count == 32) {
    f.write(reinterpret_cast<const char*>(&color_header), sizeof(color_header));
  }
  f.write(reinterpret_cast<const char*>(data), imageSize());
}

} // myyuv
