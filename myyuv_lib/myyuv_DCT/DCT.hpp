#pragma once

#include <array>
#include <cstdint>
#include "myyuv_yuv.hpp"

namespace myyuvDCT {

/**
* @brief DCT compression for YUV in planar format.
* @note The higher quality is, the less effective compression will be, but more details will be preserved.
* @param yuv YUV image to compress
* @param params Parameters for DCT compression: the quality that ranges from 1 to 100.
* @return New compressed image.
*/
myyuv::YUV compress_DCT_planar(const myyuv::YUV& yuv, const std::array<uint8_t, 3>& params);

/**
* @brief DCT decompression for YUV in planar format.
* @warning The parameters should be exactly the same as used in compression. The function does not check if parameters match with `compression_params`
* @param yuv YUV image to decompress.
* @param params Parameters for DCT compression: the quality that ranges from 1 to 100.
* @return New decompressed image.
*/
myyuv::YUV decompress_DCT_planar(const myyuv::YUV& yuv, const std::array<uint8_t, 3>& params);

} // myyuvDCT
