#pragma once

#include <array>
#include <cstdint>
#include "myyuv_yuv.hpp"

namespace myyuvDCT {

myyuv::YUV compress_DCT_planar(const myyuv::YUV& yuv, const std::array<uint8_t, 3>& params);

myyuv::YUV decompress_DCT_planar(const myyuv::YUV& yuv, const std::array<uint8_t, 3>& params);

} // myyuvDCT
