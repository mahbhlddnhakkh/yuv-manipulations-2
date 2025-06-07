#pragma once

#include <string>
#include <cstdint>

namespace myyuv {

#pragma pack(push, 1)
/**
* @brief Header of BMP file. It includes `File Header` and `Info Header`.
*/
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

/**
* @brief BMP Color header.
*/
struct BMPColorHeader {
  uint32_t red_mask = 0x00ff0000;
  uint32_t green_mask = 0x0000ff00;
  uint32_t blue_mask = 0x000000ff;
  uint32_t alpha_mask = 0xff000000;
  uint32_t color_space = 0x73524742; // sRGB
  uint32_t unused[16] = { 0 };
};
#pragma pack(pop)

/**
* @brief Class that represents BMP (RGB[A]) image.
* @var header BMP file header (file header and info header).
* @var color_header BMP color header.
* @var data Image data.
*/
class BMP {
public:
  BMPHeader header;
  BMPColorHeader color_header;
  uint8_t* data = nullptr;
public:
  /**
  * @brief Default empty constructor.
  * @warning If left as it is, consideres invalid.
  * @see load
  */
  BMP() {}

  /**
  * @brief Constructor that loads BMP image from file.
  * @param path Image file path.
  * @see load
  */
  explicit BMP(const std::string& path);

  /**
  * @brief Copy constructor.
  */
  BMP(const BMP& bmp);

  /**
  * @brief Copy assignment operator.
  */
  BMP& operator=(const BMP& bmp);

  /**
  * @brief Move constructor.
  */
  BMP(BMP&& bmp) noexcept;

  /**
  * @brief Move assignment operator.
  */
  BMP& operator=(BMP&& bmp) noexcept;

  /**
  * @brief Destructor.
  */
  ~BMP();

  /**
  * @brief Get absolute value of image width.
  * @return Positive image width.
  */
  uint32_t trueWidth() const noexcept;

  /**
  * @brief Get absolute value of image height.
  * @return Positive image height.
  */
  uint32_t trueHeight() const noexcept;

  /**
  * @brief Calculates image size.
  * @return Calculated image size.
  */
  uint32_t imageSize() const noexcept;

  /**
  * @brief Get color data with top-left origin.
  * @note Free the allocated memory with `delete[]`.
  * @return Allocated data memory.
  */
  uint8_t* colorData() const;

  /**
  * @brief Get color data with bottom-left origin.
  * @note Free the allocated memory with `delete[]`.
  * @return Allocated data memory.
  */
  uint8_t* colorDataFlipped() const;

  /**
  * @brief Check if BMP image is valid.
  * @warning The check is not perfect.
  * @return `true` if image is valid, `false` otherwise.
  * @see isValidHeader
  */
  bool isValid() const noexcept;

  /**
  * @brief Check if BMP image header is valid.
  * @warning The check is not perfect.
  * @return `true` if image header is valid, `false` otherwise.
  * @see isValid
  */
  bool isValidHeader() const noexcept;

  /**
  * @brief Loads BMP image from file.
  * @note The object won't be modifed on exception (exception safe).
  * @param path Image file path.
  */
  void load(const std::string& path);

  /**
  * @brief Dumps image to file.
  * @param path Path to dump.
  */
  void dump(const std::string& path) const;
};

} // myyuv
