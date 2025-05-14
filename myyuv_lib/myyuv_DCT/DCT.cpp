#include "DCT.hpp"

#include "myyuv_yuv.hpp"
#include "Huffman.hpp"
#include <stdexcept>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <vector>
#ifdef MYYUV_USE_OPENMP
#include <exception>
#include <omp.h>
#endif

namespace {

struct DCTYUVPlane {
  uint32_t chunks_sizes_size;
  uint32_t content_size;
  uint8_t* chunks_sizes = nullptr;
  uint8_t* content = nullptr;
  std::vector<uint32_t> getContentPos() const {
    assert(chunks_sizes_size > 0);
    //assert(content_size > 0);
    assert(chunks_sizes);
    //assert(content);
    std::vector<uint32_t> res(chunks_sizes_size);
    res[0] = 0;
    for (uint32_t i = 1; i < chunks_sizes_size; i++) {
      res[i] = res[i - 1] + chunks_sizes[i - 1];
    }
    //assert(res[chunks_sizes_size - 1] + chunks_sizes[chunks_sizes_size - 1] == content_size);
    return res;
  }
  uint32_t totalSize() const noexcept {
    assert(chunks_sizes_size > 0);
    assert(content_size > 0);
    return 2 * sizeof(uint32_t) + chunks_sizes_size + content_size;
  }
  static DCTYUVPlane load(const uint8_t* data, uint32_t size) {
    uint32_t offset = 2 * sizeof(uint32_t);
    if (size <= offset) {
      throw std::runtime_error("DCTYUVPlane load bad size");
    }
    DCTYUVPlane res;
    std::copy(data, data + sizeof(res.chunks_sizes_size), reinterpret_cast<uint8_t*>(&res.chunks_sizes_size));
    std::copy(data + sizeof(res.chunks_sizes_size), data + sizeof(res.chunks_sizes_size) + sizeof(res.content_size), reinterpret_cast<uint8_t*>(&res.content_size));
    if (res.chunks_sizes_size <= 0) {
      throw std::runtime_error("DCTYUVPlane load chunks_sizes_size bad size");
    }
    if (res.content_size <= 0) {
      throw std::runtime_error("DCTYUVPlane load content_size bad size");
    }
    if (size < res.totalSize()) {
      throw std::runtime_error("DCTYUVPlane load bad size");
    }
    res.chunks_sizes = new uint8_t[res.chunks_sizes_size];
    std::copy(data + offset, data + offset + res.chunks_sizes_size, res.chunks_sizes);
    offset += res.chunks_sizes_size;
    res.content = new uint8_t[res.content_size];
    std::copy(data + offset, data + offset + res.content_size, res.content);
    return res;
  }
  void dumpTo(uint8_t* data, uint32_t size) const {
    assert(totalSize() <= size);
    assert(chunks_sizes_size > 0);
    assert(content_size > 0);
    assert(chunks_sizes);
    assert(content);
    std::copy(reinterpret_cast<const uint8_t*>(&chunks_sizes_size), reinterpret_cast<const uint8_t*>(&chunks_sizes_size) + sizeof(chunks_sizes_size), data);
    std::copy(reinterpret_cast<const uint8_t*>(&content_size), reinterpret_cast<const uint8_t*>(&content_size) + sizeof(content_size), data + sizeof(chunks_sizes_size));
    std::copy(chunks_sizes, chunks_sizes + chunks_sizes_size, data + 2 * sizeof(uint32_t));
    std::copy(content, content + content_size, data + totalSize() - content_size);
  }
  DCTYUVPlane() {}
  ~DCTYUVPlane() {
    delete[] chunks_sizes;
    delete[] content;
  }
  DCTYUVPlane(DCTYUVPlane&& dct_plane) {
    operator=(std::move(dct_plane));
  }
  DCTYUVPlane& operator=(DCTYUVPlane&& dct_plane) {
    std::swap(chunks_sizes_size, dct_plane.chunks_sizes_size);
    std::swap(content_size, dct_plane.content_size);
    std::swap(chunks_sizes, dct_plane.chunks_sizes);
    std::swap(content, dct_plane.content);
    return *this;
  }
  bool operator==(const DCTYUVPlane& dct_plane) const noexcept {
    if (chunks_sizes == nullptr || dct_plane.chunks_sizes == nullptr || content == nullptr || dct_plane.content == nullptr) {
      return chunks_sizes == dct_plane.chunks_sizes && content == dct_plane.content;
    }
    if (chunks_sizes_size != dct_plane.chunks_sizes_size) {
      return false;
    }
    if (content_size != dct_plane.content_size) {
      return false;
    }
    if (!std::equal(chunks_sizes, chunks_sizes + chunks_sizes_size, dct_plane.chunks_sizes)) {
      return false;
    }
    if (!std::equal(content, content + content_size, dct_plane.content)) {
      return false;
    }
    return true;
  }
  bool operator!=(const DCTYUVPlane& dct_plane) const noexcept {
    return !operator==(dct_plane);
  }
};

struct DCTYUV {
  uint32_t planes_sizes[3] = { 0 };
  DCTYUVPlane planes[3];
  uint32_t totalSize() const noexcept {
    assert([this]()->bool{
      for (uint32_t i = 0; i < 3; i++) {
        if (planes_sizes[i] != 0 && planes[i].totalSize() != planes_sizes[i]) {
          return false;
        }
      }
      return true;
    }());
    uint32_t res = sizeof(planes_sizes);
    for (uint32_t i = 0; i < 3; i++) {
      res += planes_sizes[i];
    }
    return res;
  }
  static DCTYUV load(const uint8_t* data, uint32_t size) {
    uint32_t offset = sizeof(planes_sizes);
    if (size <= offset) {
      throw std::runtime_error("DCTYUV load bad size");
    }
    DCTYUV res;
    const uint32_t* _data = reinterpret_cast<const uint32_t*>(data);
    std::copy(_data, _data + sizeof(planes_sizes) / sizeof(*planes_sizes), res.planes_sizes);
    {
      uint32_t sz = offset;
      for (uint32_t i = 0; i < 3; i++) {
        sz += res.planes_sizes[i];
      }
      if (size < sz) {
        throw std::runtime_error("DCTYUV load bad size");
      }
    }
    uint32_t planes_pos[3];
    planes_pos[0] = sizeof(planes_sizes);
    for (uint32_t i = 1; i < 3; i++) {
      planes_pos[i] = planes_pos[i - 1] + res.planes_sizes[i - 1];
    }
    for (uint32_t i = 0; i < 3; i++) {
      if (res.planes_sizes[i] != 0) {
        res.planes[i] = DCTYUVPlane::load(data + planes_pos[i], res.planes_sizes[i]);
        res.planes_sizes[i] = res.planes[i].totalSize();
      }
    }
    return res;
  }
  uint8_t* dump() const {
    uint32_t sz = totalSize();
    uint8_t* res = new uint8_t[sz];
    std::copy(planes_sizes, planes_sizes + sizeof(planes_sizes) / sizeof(*planes_sizes), reinterpret_cast<uint32_t*>(res));
    uint32_t offset = sizeof(planes_sizes);
    for (uint32_t i = 0; i < 3; i++) {
      if (planes_sizes[i] != 0) {
        assert(planes_sizes[i] == planes[i].totalSize());
        planes[i].dumpTo(res + offset, planes_sizes[i]);
        offset += planes_sizes[i];
      }
    }
    return res;
  }
  DCTYUV() {}
  DCTYUV(DCTYUV&& dct) {
    operator=(std::move(dct));
  }
  DCTYUV& operator=(DCTYUV&& dct) {
    std::swap(planes_sizes, dct.planes_sizes);
    std::swap(planes, dct.planes);
    return *this;
  }
  bool operator==(const DCTYUV& dct) const noexcept {
    if (!std::equal(planes_sizes, planes_sizes + sizeof(planes_sizes) / sizeof(*planes_sizes), dct.planes_sizes)) {
      return false;
    }
    for (uint32_t i = 0; i < 3; i++) {
      if (planes[i] != dct.planes[i]) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const DCTYUV& dct) const noexcept {
    return !operator==(dct);
  }
};

} // namespace anon

static constexpr const float lum_q_table[64] = {
  16, 11, 10, 16, 24, 40, 51, 61,
  12, 12, 14, 19, 26, 58, 60, 55,
  14, 13, 16, 24, 40, 57, 69, 56,
  14, 17, 22, 29, 51, 87, 80, 62,
  18, 22, 37, 56, 68, 109, 103, 77,
  24, 35, 55, 64, 81, 104, 113, 92,
  49, 64, 78, 87, 103, 121, 120, 101,
  72, 92, 95, 98, 112, 100, 103, 99,
};

static constexpr const float chroma_q_table[64] = {
  17, 18, 24, 47, 99, 99, 99, 99,
  18, 21, 26, 66, 99, 99, 99, 99,
  24, 26, 56, 99, 99, 99, 99, 99,
  47, 66, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
  99, 99, 99, 99, 99, 99, 99, 99,
};

static constexpr const float DCT_matrix8[64] = {
  0.3535533845424652f, 0.3535533845424652f, 0.3535533845424652f, 0.3535533845424652f, 0.3535533845424652f, 0.3535533845424652f, 0.3535533845424652f, 0.3535533845424652f, 
  0.4903925955295563f, 0.4157347679138184f, 0.277785062789917f, 0.09754510968923569f, -0.09754515439271927f, -0.2777851521968842f, -0.4157347977161407f, -0.4903926253318787f, 
  0.4619397222995758f, 0.1913416981697083f, -0.1913417428731918f, -0.4619397819042206f, -0.4619397222995758f, -0.1913415491580963f, 0.1913417875766754f, 0.4619397521018982f, 
  0.4157347679138184f, -0.09754515439271927f, -0.4903926253318787f, -0.2777849733829498f, 0.2777851819992065f, 0.4903925955295563f, 0.09754502773284912f, -0.4157348573207855f, 
  0.3535533547401428f, -0.3535533547401428f, -0.353553295135498f, 0.3535534739494324f, 0.3535533547401428f, -0.3535535931587219f, -0.3535532355308533f, 0.3535533845424652f, 
  0.277785062789917f, -0.4903926253318787f, 0.09754519909620285f, 0.4157346487045288f, -0.4157348573207855f, -0.09754510223865509f, 0.4903926253318787f, -0.2777853906154633f, 
  0.1913416981697083f, -0.4619397222995758f, 0.4619397521018982f, -0.1913419365882874f, -0.1913414746522903f, 0.4619396328926086f, -0.4619398415088654f, 0.1913419365882874f, 
  0.09754510968923569f, -0.2777849733829498f, 0.4157346487045288f, -0.4903925657272339f, 0.4903926849365234f, -0.4157347679138184f, 0.2777855396270752f, -0.09754576534032822f,
};

template<int size>
static void squareMatrixMul(const float a[size * size], const float b[size * size], float c[size * size]) noexcept {
  std::fill_n(c, size * size, 0.0f);
  for (int i = 0; i < size; i++) {
    for (int k = 0; k < size; k++) {
      for (int j = 0; j < size; j++) {
        c[i * size + j] += a[k + i * size] * b[j + k * size];
      }
    }
  }
}

template<int size>
static void squareMatrixMulT(const float a[size * size], const float bT[size * size], float c[size * size]) noexcept {
  std::fill_n(c, size * size, 0.0f);
  for (int i = 0; i < size; i++) {
    for (int k = 0; k < size; k++) {
      for (int j = 0; j < size; j++) {
        c[i * size + j] += a[k + i * size] * bT[k + j * size];
      }
    }
  }
}

template<int size>
static void squareMatrixMulT2(const float aT[size * size], const float b[size * size], float c[size * size]) noexcept {
  std::fill_n(c, size * size, 0.0f);
  for (int i = 0; i < size; i++) {
    for (int k = 0; k < size; k++) {
      for (int j = 0; j < size; j++) {
        c[i * size + j] += aT[i + k * size] * b[j + k * size];
      }
    }
  }
}

// data_block will be lost!
static void applyDCTBlock(float data_block[64], int16_t res[64], const float q_table[64]) noexcept {
  float data_block_2[64];
  squareMatrixMul<8>(DCT_matrix8, data_block, data_block_2);
  squareMatrixMulT<8>(data_block_2, DCT_matrix8, data_block);
  for (int i = 0; i < 64; i++) {
    res[i] = static_cast<int16_t>(std::round(data_block[i] / q_table[i]));
  }
}

static void applyDCTPlane(DCTYUVPlane& res, const uint8_t* data, uint32_t width, uint32_t height, float q, const float q_50_table[64]) {
  if (width % 8 != 0) {
    throw std::runtime_error("Error. width % 8 must be 0");
  }
  if (height % 8 != 0) {
    throw std::runtime_error("Error. height % 8 must be 0");
  }
  float q_table[64];
  const float q_table_mul = (q >= 50.5f) ? (100.0f - q) / 50.0f : 50.0f / q;
  for (uint32_t i = 0; i < 64; i++) {
    q_table[i] = std::clamp(std::round(q_50_table[i] * q_table_mul), 1.0f, 255.0f);
  }
  res.chunks_sizes_size = width * height / 64;
  res.chunks_sizes = new uint8_t[res.chunks_sizes_size];
  uint8_t** contents = new uint8_t*[res.chunks_sizes_size];
#ifdef MYYUV_USE_OPENMP
  #pragma omp parallel for schedule(dynamic, 1) collapse(2)
#endif
  for (uint32_t j = 0; j < height; j += 8) {
    for (uint32_t i = 0; i < width; i += 8) {
      int16_t block_res[64];
      float data_block[64];
      for (uint32_t jj = 0; jj < 8; jj++) {
        for (uint32_t ii = 0; ii < 8; ii++) {
          data_block[ii + jj * 8] = static_cast<float>(data[(i + ii) + (j + jj) * width]) - 128.0f;
        }
      }
      applyDCTBlock(data_block, block_res, q_table);
      Huffman huffman = Huffman::fromData(block_res);
      const uint32_t k = (i + j * width / 8) / 8;
      assert(k < res.chunks_sizes_size);
      huffman.dump(contents[k], res.chunks_sizes[k]);
      assert(res.chunks_sizes[k]);
    }
  }
  std::vector<uint32_t> content_pos = res.getContentPos();
  res.content_size = content_pos[res.chunks_sizes_size - 1] + res.chunks_sizes[res.chunks_sizes_size - 1];
  assert(res.content_size > 0);
  res.content = new uint8_t[res.content_size];
  for (uint32_t i = 0; i < res.chunks_sizes_size; i++) {
    std::copy(contents[i], contents[i] + res.chunks_sizes[i], res.content + content_pos[i]);
    delete[] contents[i];
  }
  delete[] contents;
}

static void restoreDCTBlock(float block_res[64], const uint8_t* huffman_data, uint8_t huffman_size, const float q_table[64]) {
  const Huffman huffman = Huffman::fromDump(huffman_data, huffman_size);
  float data_block[64];
  for (uint32_t i = 0; i < 64; i++) {
    block_res[i] = static_cast<float>(huffman.data[i]) * q_table[i];
  }
  squareMatrixMulT2<8>(DCT_matrix8, block_res, data_block);
  squareMatrixMul<8>(data_block, DCT_matrix8, block_res);
}

static void restoreDCTPlane(uint8_t* res, const DCTYUVPlane& dct, uint32_t width, uint32_t height, float q, const float q_50_table[64]) {
  if (width % 8 != 0) {
    throw std::runtime_error("Error. width % 8 must be 0");
  }
  if (height % 8 != 0) {
    throw std::runtime_error("Error. height % 8 must be 0");
  }
  float q_table[64];
  const float q_table_mul = (q >= 50.5f) ? (100.0f - q) / 50.0f : 50.0f / q;
  for (uint32_t i = 0; i < 64; i++) {
    q_table[i] = std::clamp(std::round(q_50_table[i] * q_table_mul), 1.0f, 255.0f);
  }
  std::vector<uint32_t> contents = dct.getContentPos();
#ifdef MYYUV_USE_OPENMP
  #pragma omp parallel for schedule(dynamic, 1) collapse(2)
#endif
  for (uint32_t j = 0; j < height; j += 8) {
    for (uint32_t i = 0; i < width; i += 8) {
      const uint32_t k = (i + j * width / 8) / 8;
      float block_res[64];
      restoreDCTBlock(block_res, dct.content + contents[k], dct.chunks_sizes[k], q_table);
      for (uint32_t jj = 0; jj < 8; jj++) {
        for (uint32_t ii = 0; ii < 8; ii++) {
          res[(i + ii) + (j + jj) * width] = std::clamp(static_cast<int>(std::round(block_res[ii + jj * 8])) + 128, 0, UINT8_MAX);
        }
      }
    }
  }
}

namespace myyuv {

YUV compress_DCT_planar(const YUV& yuv, const std::array<uint8_t, 3>& params) {
  if (yuv.getFormatGroup() != YUV::FormatGroup::PLANAR) {
    throw std::runtime_error("Error compressing: YUV must be planar");
  }
  if (yuv.getCompression() != YUV::Compression::NONE) {
    throw std::runtime_error("Error compressing: can't compress uncompressed YUV");
  }
  for (uint32_t i = 0; i < 3; i++) {
    if (params[i] < 1 || params[i] > 100) {
      throw std::runtime_error("Level of quality must be between 1 and 100");
    }
  }
  auto fractions = yuv.getResolutionFraction();
  assert(yuv.header.width % (8 * fractions[0]) == 0);
  assert(yuv.header.height % (8 * fractions[1]) == 0);
  auto planes = yuv.getYUVPlanes();
  assert(YUV::max_planes == 3 || YUV::max_planes > 3 && planes[3] == nullptr); // Transparency is not supported
  assert(planes[0] != nullptr && planes[1] != nullptr && planes[2] != nullptr); // not ready to handle when one plane is missing
  YUV res;
  res.header = yuv.header;
  res.header.compression = static_cast<uint16_t>(YUV::Compression::DCT);
  res.header.compression_params_size = 3;
  res.header.compression_params_pos = sizeof(res.header);
  res.header.data_pos = sizeof(res.header) + 3;
  res.compression_params = new uint8_t[3];
  std::copy(params.data(), params.data() + 3, res.compression_params);
  static constexpr const float* tables[3] = { lum_q_table, chroma_q_table, chroma_q_table };
  DCTYUV dct;
#ifdef MYYUV_USE_OPENMP
  int omp_nested_prev = omp_get_nested();
  std::exception_ptr omp_exception;
  omp_set_nested(1);
  #pragma omp parallel for
#endif
  for (uint8_t i = 0; i < 3; i++) {
#ifdef MYYUV_USE_OPENMP
    try {
#endif
    auto width_height = yuv.getWidthHeightChannel(i);
    applyDCTPlane(dct.planes[i], planes[i], width_height[0], width_height[1], params[i], tables[i]);
    dct.planes_sizes[i] = dct.planes[i].totalSize();
#ifdef MYYUV_USE_OPENMP
    } catch (...) {
      #pragma omp critical
      if (!omp_exception) {
        omp_exception = std::current_exception();
      }
    }
#endif
  }
#ifdef MYYUV_USE_OPENMP
  omp_set_nested(omp_nested_prev);
  if (omp_exception) {
    std::rethrow_exception(omp_exception);
  }
#endif
  res.header.data_size = dct.totalSize();
  res.data = dct.dump();
  return res;
}

YUV decompress_DCT_planar(const YUV& yuv, const std::array<uint8_t, 3>& params) {
  if (yuv.getFormatGroup() != YUV::FormatGroup::PLANAR) {
    throw std::runtime_error("Error decompressing: YUV must be planar");
  }
  assert(yuv.header.compression_params_size == 3);
  assert(yuv.compression_params);
  for (uint32_t i = 0; i < 3; i++) {
    if (params[i] < 1 || params[i] > 100) {
      throw std::runtime_error("Level of quality must be between 1 and 100");
    }
  }
  auto fractions = yuv.getResolutionFraction();
  assert(yuv.header.width % (8 * fractions[0]) == 0);
  assert(yuv.header.height % (8 * fractions[1]) == 0);
  YUV res;
  res.header = yuv.header;
  res.header.compression = static_cast<uint16_t>(YUV::Compression::NONE);
  res.header.compression_params_size = 0;
  res.header.compression_params_pos = 0;
  res.header.data_pos = sizeof(yuv.header);
  res.compression_params = nullptr;
  res.header.data_size = yuv.getImageSize();
  DCTYUV dct = DCTYUV::load(yuv.data, yuv.header.data_size);
  res.data = new uint8_t[res.header.data_size];
  auto planes = res.getYUVPlanes();
  assert(YUV::max_planes == 3 || YUV::max_planes > 3 && planes[3] == nullptr); // Transparency is not supported
  assert(planes[0] != nullptr && planes[1] != nullptr && planes[2] != nullptr); // not ready to handle when one plane is missing
  static constexpr const float* tables[3] = { lum_q_table, chroma_q_table, chroma_q_table };
#ifdef MYYUV_USE_OPENMP
  int omp_nested_prev = omp_get_nested();
  std::exception_ptr omp_exception;
  omp_set_nested(1);
  #pragma omp parallel for
#endif
  for (uint8_t i = 0; i < 3; i++) {
#ifdef MYYUV_USE_OPENMP
    try {
#endif
    auto width_height = yuv.getWidthHeightChannel(i);
    restoreDCTPlane(planes[i], dct.planes[i], width_height[0], width_height[1], params[i], tables[i]);
#ifdef MYYUV_USE_OPENMP
    } catch (...) {
      #pragma omp critical
      if (!omp_exception) {
        omp_exception = std::current_exception();
      }
    }
#endif
  }
#ifdef MYYUV_USE_OPENMP
  omp_set_nested(omp_nested_prev);
  if (omp_exception) {
    std::rethrow_exception(omp_exception);
  }
#endif
  return res;
}

} // myyuv
