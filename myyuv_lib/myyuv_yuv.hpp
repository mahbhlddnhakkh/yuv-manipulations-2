#pragma once

#include "myyuv_bmp.hpp"

#include <string>
#include <unordered_map>
#include <functional>
#include <array>
#include <cstdint>

namespace myyuv {

#pragma pack(push, 1)
/**
* @brief Header of `myyuv` file.
*/
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

/**
* @brief Class that represents YUV image.
*/
class YUV {
public:
  YUVHeader header;
  uint8_t* compression_params = nullptr;
  uint8_t* data = nullptr;
public:
  /**
  * @brief Enum for YUV format group: either packed or planar.
  */
  enum class FormatGroup { UNKNOWN = 0, PACKED, PLANAR };

  /**
  * @brief Alias for fourcc format
  */
  using FourccFormat = uint32_t;

  /**
  * @brief Default fourcc formats.
  */
  struct FourccFormats {
    static constexpr const FourccFormat UNKNOWN = 0;
    static constexpr const FourccFormat IYUV = 0x56555949;
  };

  /**
  * @brief Alias for compression.
  */
  using Compression = uint16_t;

  /**
  * @brief Default compressions.
  */
  struct Compressions {
    static constexpr const Compression NONE = 0;
    static constexpr const Compression DCT = 1;
  };

  /**
  * @brief Maximum amount of YUV planes for planar group.
  */
  static constexpr const uint32_t max_planes = 4;

  /**
  * @brief No plane indication for YUV plane for planar group.
  */
  static constexpr const uint8_t no_plane = 0xff;

  /**
  * @brief Map for YUV order for planes for planar group.
  * @note Use `no_pane` if plane is unused.
  * @example YUV -> [0, 1, 2] ; YVU -> [0, 2, 1]
  */
  static std::unordered_map<FourccFormat, std::array<uint8_t, max_planes>> yuv_order_planes_map;

  /**
  * @brief Map for fraction of width and height for chroma subsampling.
  * @example IYUV -> [2, 2] // because IYUV is 4:2:0 (has half width and half height)
  */
  static std::unordered_map<FourccFormat, std::array<uint32_t, 2>> yuv_resolution_fraction_map;

  /**
  * @brief Map for converting BMP RGB image to YUV image.
  */
  static std::unordered_map<FourccFormat, std::function<YUV(const BMP&)>> bmp_to_yuv_map;

  /**
  * @brief Map for compressing YUV image.
  */
  static std::unordered_map<Compression, std::unordered_map<FourccFormat, std::function<YUV(const YUV&, const void*, uint32_t)>>> compress_map;

  /**
  * @brief Map for decompressing YUV image.
  */
  static std::unordered_map<Compression, std::unordered_map<FourccFormat, std::function<YUV(const YUV&)>>> decompress_map;

  /**
  * @brief Map for identifying YUV format group.
  * @see FormatGroup
  */
  static std::unordered_map<FourccFormat, FormatGroup> yuv_format_group_map;

  /**
  * @brief Default empty constructor.
  * @warning If left as it is, consideres invalid.
  * @see load
  */
  YUV() {}

  /**
  * @brief Constructor that loads YUV image from file.
  * @param path Image file path.
  * @see load
  */
  explicit YUV(const std::string& path);

  /**
  * @brief Constructor that converts BMP RGB(A) image to YUV image.
  * @param bmp BMP image.
  * @param format Requested fourcc format.
  * @see load
  */
  explicit YUV(const BMP& bmp, FourccFormat format);

  /**
  * @brief Copy constructor.
  */
  YUV(const YUV& yuv);

  /**
  * @brief Copy assignment operator.
  */
  YUV& operator=(const YUV& yuv);

  /**
  * @brief Move constructor.
  */
  YUV(YUV&& yuv) noexcept;

  /**
  * @brief Move assignment operator.
  */
  YUV& operator=(YUV&& yuv) noexcept;

  /**
  * @brief Destructor.
  */
  ~YUV();

  /**
  * @brief Check if YUV image is valid.
  * @warning The check is not perfect.
  * @return `true` if image is valid, `false` otherwise.
  * @see isValidHeader
  */
  bool isValid() const noexcept;

  /**
  * @brief Check if YUV image header is valid.
  * @warning The check is not perfect.
  * @return `true` if image header is valid, `false` otherwise.
  * @see isValid
  */
  bool isValidHeader() const noexcept;

  /**
  * @brief Checks if specific fourcc format is implemented in all required maps.
  * @param format Requested fourcc format to check.
  * @param compression Requested compression to check. Can be 0 if no need to check for specific compression (default value).
  */
  static bool isImplementedFormat(FourccFormat format, Compression compression = Compressions::NONE) noexcept;

  /**
  * @brief Get image fourcc format.
  * @return fourcc format.
  * @see FourccFormat
  * @see FourccFormats
  */
  FourccFormat getFourccFormat() const noexcept;

  /**
  * @brief Get image compression.
  * @return compression.
  * @see Compression
  * @see Compressions
  */
  Compression getCompression() const noexcept;

  /**
  * @brief Get image width.
  * @return image width.
  */
  uint32_t getWidth() const noexcept;

  /**
  * @brief Get image height.
  * @return image height.
  */
  uint32_t getHeight() const noexcept;

  /**
  * @brief Get image data size.
  * @note It just reads `data_size` from the header.
  * @return image data size.
  */
  uint32_t getDataSize() const noexcept;

  /**
  * @brief Fraction of width and height for chroma subsampling.
  * @example IYUV -> [2, 2] // because IYUV is 4:2:0 (has half width and half height).
  * @return Array of 2 numbers: fraction for width and height.
  * @see yuv_resolution_fraction_map
  */
  std::array<uint32_t, 2> getResolutionFraction() const;

  /**
  * @brief Get width and height for specific color channel with appropriate subsampling.
  * @note First color channel is 0.
  * @note Only applying subsampling to channels 1 and 2.
  * @return Array of 2 numbers: width and height.
  */
  std::array<uint32_t, 2> getWidthHeightChannel(uint8_t channel) const;

  /**
  * @brief Bits for plane per pixel.
  * @note Order of planes is YUV(A).
  * @return Array of `max_planes` numbers: bits for each plane.
  */
  std::array<uint32_t, max_planes> getFormatSizeBits() const;

  /**
  * @brief YUV order for planes for planar group.
  * @example YUV -> [0, 1, 2] ; YVU -> [0, 2, 1]
  * @note `no_plane` is used if the plane is unused.
  * @return Array on indexes.
  * @see yuv_order_planes_map
  */
  std::array<uint8_t, max_planes> getYUVPlanesOrder() const;

  /**
  * @brief Calculates real image size.
  * @note Ignores `data_size` from header.
  * @return Calculated image size.
  */
  uint32_t getImageSize() const;

  /**
  * @brief Get pointers to YUV planes in `data`.
  * @return Array of pointers to `data`.
  */
  std::array<const uint8_t*, max_planes> getYUVPlanes() const;

  /**
  * @brief Get pointers to YUV planes in `data`.
  * @return Array of pointers to `data`.
  */
  std::array<uint8_t*, max_planes> getYUVPlanes();

  /**
  * @brief Get format group for image: either packed or planar.
  * @return Format group.
  * @see FormatGroup
  */
  FormatGroup getFormatGroup() const noexcept;

  /**
  * @brief Get format group for specified fourcc format: either packed or planar.
  * @param format Requested fourcc format.
  * @return Format group.
  * @see FormatGroup
  */
  static FormatGroup getFormatGroup(FourccFormat format) noexcept;

  /**
  * @brief Compresses YUV image.
  * @warning In order do compress the image, the image must be decompressed first.
  * @param compression Requested compression.
  * @param params Compression params data.
  * @param params_size Compression params data size in bytes.
  * @return New compressed YUV image.
  * @see Compressions
  * @see compress_map
  */
  YUV compress(Compression compression, const void* params, uint32_t params_size) const;

  /**
  * @brief Decompresses YUV image.
  * @note If image is not compressed, returns the copy of the image.
  * @return New decompressed image.
  * @see decompress_map
  */
  YUV decompress() const;

  /**
  * @brief Checks if image is compressed.
  * @return `true` if image is compressed, `false` otherwise.
  */
  bool isCompressed() const noexcept;

  /**
  * @brief Loads YUV image from file.
  * @note The object won't be modifed on exception (exception safe).
  * @param path Image file path.
  */
  void load(const std::string& path);

  /**
  * @brief Converts BMP RGB(A) image to YUV image.
  * @note The object won't be modifed on exception (exception safe).
  * @param bmp BMP image.
  * @param format Requested fourcc format.
  * @return New YUV image
  */
  void load(const BMP& bmp, FourccFormat format);

  /**
  * @brief Dumps image to file (including compressed images).
  * @param path Path to dump.
  */
  void dump(const std::string& path) const;
};

} // myyuv
