#pragma once

#include <cstdint>
#include <memory>
#include <set>
#include <map>
#include <bitset>

namespace myyuvDCT {

struct HFMNode {
  int16_t ch;
  uint8_t freq;
  std::shared_ptr<HFMNode> left;
  std::shared_ptr<HFMNode> right;
  HFMNode(int16_t ch = 0, uint8_t freq = 0, std::shared_ptr<HFMNode> left = nullptr, std::shared_ptr<HFMNode> right = nullptr) : ch(ch), freq(freq), left(left), right(right) {}
  bool isLeaf() const noexcept {
    return left == nullptr && right == nullptr;
  }
  struct Compare {
    inline bool operator()(const std::shared_ptr<HFMNode>& a, const std::shared_ptr<HFMNode>& b) const noexcept {
      return a->freq > b->freq;
    }
  };
};

class Huffman {
public:
  Huffman(Huffman&& huffman) noexcept;
  Huffman& operator=(Huffman&& huffman) noexcept;
  static Huffman fromData(const int16_t data[64]);
  static Huffman fromDump(const uint8_t* data, uint8_t size);
  void dump(uint8_t*& res_data, uint8_t& res_size) const;
  void getData(int16_t data[64]) const;
  bool operator==(const Huffman& huffman) const noexcept;
  bool operator!=(const Huffman& huffman) const noexcept;
protected:
  friend struct HFMNode;
  Huffman() {}
  int16_t data[64] = { 0 };
  uint16_t encoded_data_bits = 0;
  std::bitset<512> encoded_data;
  std::map<uint8_t, std::set<int16_t>> tree_data;
};

} // myyuvDCT
