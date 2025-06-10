#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <bitset>

namespace myyuvDCT {

/** 
* @brief Tree node for the Huffman tree.
*/
struct HFMNode {
  int16_t ch; /// Character that is stored in the node.
  uint8_t freq; /// Frequency of a character.
  std::shared_ptr<HFMNode> left; /// Left node in the Huffman tree.
  std::shared_ptr<HFMNode> right; /// Right node in the Huffman tree.

  /**
  * @brief Constructor
  * @param ch Character that is stored in the node.
  * @param freq Frequency of a character.
  * @param left Left node in the Huffman tree.
  * @param right Right node in the Huffman tree.
  */
  HFMNode(int16_t ch = 0, uint8_t freq = 0, std::shared_ptr<HFMNode> left = nullptr, std::shared_ptr<HFMNode> right = nullptr) : ch(ch), freq(freq), left(left), right(right) {}
  bool isLeaf() const noexcept {
    return left == nullptr && right == nullptr;
  }

  /**
  * @brief Struct that is used to compare nodes in priority queue. The nodes are compared by it's frequency of it's character.
  * 
  */
  struct Compare {
    inline bool operator()(const std::shared_ptr<HFMNode>& a, const std::shared_ptr<HFMNode>& b) const noexcept {
      return a->freq > b->freq;
    }
  };
};

/**
* @brief Class that handles Huffman coding for 8x8 matrix block.
* @note Create objects only with `fromData` or `fromDump`.
* @see fromData
* @see fromDump
*/
class Huffman {
public:
  /**
  * @brief Move constructor.
  */
  Huffman(Huffman&& huffman) noexcept;

  /**
  * @brief Move assignment operator.
  */
  Huffman& operator=(Huffman&& huffman) noexcept;

  /**
  * @brief Constructs object from 8x8 matrix block.
  * @param data 8x8 matrix in a vector form.
  * @return Constructed Huffman object.
  */
  static Huffman fromData(const int16_t data[64]);

  /**
  * @brief Constructs object from it's dump.
  * @param data Dump data.
  * @return Dump data in bytes.
  */
  static Huffman fromDump(const uint8_t* data, uint8_t size);

  /**
  * @brief Dumps object to `res_data` with `res_size` in bytes.
  * @param[out] res_data Object dump.
  * @param[out] res_size Object dump size in bytes.
  */
  void dump(uint8_t*& res_data, uint8_t& res_size) const;

  /**
  * @brief Get 8x8 matrix block.
  * @param[out] data Dumps matrix to this array in a vector form.
  */
  void getData(int16_t data[64]) const;

  /**
  * @brief Compares Huffman objects. It compares matrix in both in encoded and decoded format.
  * @note Useful in debugging and testing.
  * @return `true` if objects are the same, `false` otherwise.
  * @see operator!=
  */
  bool operator==(const Huffman& huffman) const noexcept;

  /**
  * @brief Compares Huffman objects. It compares matrix in both in encoded and decoded format.
  * @note Useful in debugging and testing.
  * @return `false` if objects are the same, `true` otherwise.
  * @see operator==
  */
  bool operator!=(const Huffman& huffman) const noexcept;
protected:
  friend struct HFMNode;
  /// Default constructor is not allowed, use `fromData` and `fromDump`
  Huffman() {}

  /// matrix 8x8 in a vector form
  int16_t data[64] = { 0 };
  uint16_t encoded_data_bits = 0;
  std::bitset<512> encoded_data;

  /// map<code_length, vector<character>>
  std::map<uint8_t, std::vector<int16_t>> tree_data;
};

} // myyuvDCT
