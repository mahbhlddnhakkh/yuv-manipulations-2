#pragma once

#include <array>
#include <cstdint>

namespace myyuv {

class YUV;

YUV compress_DCT_planar(const YUV& yuv, const std::array<uint8_t, 3>& params);

YUV decompress_DCT_planar(const YUV& yuv, const std::array<uint8_t, 3>& params);

} // myyuv
