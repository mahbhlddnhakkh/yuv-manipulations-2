#include "Huffman.hpp"

#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <unordered_map>
#include <queue>
#include <cassert>
#include <functional>
#include <iostream>
#include <limits>
#include <algorithm>
#include <iterator>

// https://stackoverflow.com/a/58568736
template <typename T>
static T divide_roundup(T numer, T denom) {
  static_assert(std::numeric_limits<T>::is_integer, "Only integer types are allowed");
  T result = ((numer) < 0) != ((denom) < 0) ?
    (numer) / (denom) :
    ((numer) + ((denom) < 0 ? (denom) + 1 : (denom) - 1)) / (denom);
  return result;
}

template<typename T, typename U>
inline static bool mapKeyExist(const std::map<T, U>& map, const T& key) noexcept {
  return map.find(key) != map.end();
}

static uint32_t zigzag_indexes[64] = {
  0, 8, 1, 2, 9, 16, 24, 17, 10, 3, 4, 11, 18, 25, 32, 40, 33, 26, 19, 12, 5, 6, 13, 20, 27, 34, 41, 48, 56, 49, 42, 35, 28, 21, 14, 7, 15, 22, 29, 36, 43, 50, 57, 58, 51, 44, 37, 30, 23, 31, 38, 45, 52, 59, 60, 53, 46, 39, 47, 54, 61, 62, 55, 63,
};

static void pack11bit(uint8_t* packed_res, std::set<int16_t>::iterator& it, uint8_t count) {
  std::fill(packed_res, packed_res + divide_roundup(static_cast<unsigned>(count) * 11u, 8u), 0);
  int bit_offset = 0;
  for (uint8_t i = 0; i < count; i++) {
    int byte_ind = bit_offset / 8;
    int bit_ind = bit_offset % 8;
    int16_t _num = *it;
    uint16_t num = (_num < 0 ) ? (2048 + _num) : _num;
    packed_res[byte_ind] |= (num << bit_ind) & 0xFF;
    packed_res[byte_ind + 1] |= (num >> (8 - bit_ind)) & 0xFF;
    if (bit_ind > 5) { // 16 - 11
      packed_res[byte_ind + 2] |= (num >> (16 - bit_ind)) & 0xFF;
    }
    bit_offset += 11;
    it = std::next(it);
  }
}

static void unpack11bit(const uint8_t* packed_arr, std::set<int16_t>& res, uint8_t count) {
  int bit_offset = 0;
  for (uint8_t i = 0; i < count; i++) {
    int byte_ind = bit_offset / 8;
    int bit_ind = bit_offset % 8;
    uint16_t _num = (packed_arr[byte_ind] >> bit_ind) & 0xFF;
    _num |= (packed_arr[byte_ind + 1] << (8 - bit_ind)) & 0x7FF;
    if (bit_ind > 5) { // 16 - 11
      _num |= (packed_arr[byte_ind + 2] << (16 - bit_ind)) & 0x7FF;
    }
    _num &= 0x7FF;
    int16_t num = (_num >= 1024) ? (_num - 2048) : _num;
    res.insert(num);
    bit_offset += 11;
  }
}

static void generateCodeLength(std::map<uint8_t, std::set<int16_t>>& tree_data, const std::shared_ptr<HFMNode>& node, uint8_t code_length) {
  if (node == nullptr) {
    return;
  }
  if (node->isLeaf()) {
    tree_data[code_length + (code_length == 0)].insert(node->ch);
    return;
  }
  generateCodeLength(tree_data, node->left, code_length + 1);
  generateCodeLength(tree_data, node->right, code_length + 1);
}

// map<ch, <length, code>>
static std::unordered_map<int16_t, std::pair<uint8_t, std::bitset<8>>> generateCanonicalTree(const std::map<uint8_t, std::set<int16_t>>& tree_data) {
  std::unordered_map<int16_t, std::pair<uint8_t, std::bitset<8>>> res;
  uint8_t prev_len = 0;
  uint8_t code = 0;
  for (const auto& it : tree_data) {
    const std::set<int16_t>& set = it.second;
    const uint8_t& len = it.first;
    code <<= len - prev_len;
    for (const auto& c : set) {
      assert(code < 128); // just in case
      res.insert({ c, { len, code } });
      //std::cout << (int)c << ':' << std::bitset<8>(code).to_string() << ' ' << (int)len << '\n';
      code++;
    }
    prev_len = len;
  }
  return res;
}

// https://github.com/madler/zlib/blob/develop/contrib/puff/puff.c decode (SLOW)
static int16_t decodeSymbol(uint16_t& i, const std::bitset<512>& encoded_data, const uint16_t encoded_data_bits, const std::map<uint8_t, std::set<int16_t>>& tree_data) {
  uint8_t code = 0;
  uint8_t first = 0;
  for (uint8_t j = 1; j <= 8; j++) {
    uint8_t ch_count = 0;
    const std::set<int16_t>* set = nullptr;
    if (mapKeyExist(tree_data, j)) {
      set = &tree_data.at(j);
      ch_count = set->size();
    }
    assert(i < encoded_data_bits);
    code |= encoded_data.test(i++);
    assert(static_cast<int>(ch_count) + static_cast<int>(first) < 255);
    assert(code < 255);
    if (code < ch_count + first) {
      assert(code >= first);
      assert(code - first < ch_count);
      return *std::next(set->begin(), code - first);
    }
    first += ch_count;
    first <<= 1;
    assert(code <= 127);
    code <<= 1;
  }
  throw std::runtime_error("Huffman unknown symbol");
  return 0;
}

static void decodeFromTreeData(int16_t data[64], const std::bitset<512>& encoded_data, const uint16_t encoded_data_bits, const std::map<uint8_t, std::set<int16_t>>& tree_data) {
  assert(encoded_data_bits <= encoded_data.size());
  size_t j = 0;
  uint16_t i = 0;
  while (i < encoded_data_bits) {
    assert(j < 64);
    data[zigzag_indexes[j++]] = decodeSymbol(i, encoded_data, encoded_data_bits, tree_data);
    //std::cout << "data[" << zigzag_indexes[j - 1] << "] = " << data[zigzag_indexes[j - 1]] << '\n';
    assert(i <= encoded_data_bits);
  }
  assert(i == encoded_data_bits);
  //assert(j == 64);
}

Huffman::Huffman(Huffman&& huffman) : Huffman() {
  operator=(std::move(huffman));
}

Huffman& Huffman::operator=(Huffman&& huffman) {
  std::swap(data, huffman.data);
  std::swap(encoded_data_bits, huffman.encoded_data_bits);
  std::swap(encoded_data, huffman.encoded_data);
  std::swap(tree_data, huffman.tree_data);
  return *this;
}

Huffman Huffman::fromData(const int16_t data[64]) {
  std::unordered_map<int16_t, uint8_t> freq;
  uint16_t last_seen_zero = 0;
  int16_t _data[64];
  for (size_t i = 0; i < 64; i++) {
    // zigzag path
    assert(data[i] <= 1023);
    assert(data[i] >= -1024);
    const int16_t d = data[zigzag_indexes[i]];
    _data[i] = d;
    freq[d]++;
    // collect trail of zeroes
    if (d == 0) {
      last_seen_zero++;
    } else {
      last_seen_zero = 0;
    }
  }
  uint16_t actual_msg_size = 64 - last_seen_zero;
  // remove useless zeroes.
  if (freq.find(0) != freq.end()) {
    freq.at(0) -= last_seen_zero;
  }
  if (freq[0] == 0) {
    // edge case when there are no zeroes at all
    if (actual_msg_size == 0) {
      freq.at(0) = 1;
      actual_msg_size = 1;
    } else {
      freq.erase(0);
    }
  }
  std::priority_queue<std::shared_ptr<HFMNode>, std::vector<std::shared_ptr<HFMNode>>, HFMNode::Compare> pq;
  std::shared_ptr<HFMNode> root;
  assert(root == nullptr);
  for (const auto& pair : freq) {
    pq.push(std::make_shared<HFMNode>(pair.first, pair.second));
  }
  while (pq.size() > 1) {
    std::shared_ptr<HFMNode> left = pq.top();
    pq.pop();
    std::shared_ptr<HFMNode> right = pq.top();
    pq.pop();
    root = std::make_shared<HFMNode>(0, left->freq + right->freq, left, right);
    pq.push(root);
  }
  if (root == nullptr) {
    // means only 1 ch
    root = std::make_shared<HFMNode>(_data[0], freq[_data[0]]);
  }
  assert(root != nullptr);
  Huffman huffman;
  std::copy(data, data + 64, huffman.data);
  generateCodeLength(huffman.tree_data, root, 0);
  const std::unordered_map<int16_t, std::pair<uint8_t, std::bitset<8>>> canonical_tree = generateCanonicalTree(huffman.tree_data);
  uint16_t encoded_data_bits = 0;
  for (size_t i = 0; i < actual_msg_size; i++) {
    const auto& d = canonical_tree.at(_data[i]);
    const uint8_t& code_len = d.first;
    const std::bitset<8>& code = d.second;
    for (uint8_t j = 0; j < code_len; j++) {
      huffman.encoded_data.set(encoded_data_bits + j, code.test(code_len - j - 1));
    }
    encoded_data_bits += code_len;
  }
  assert(encoded_data_bits <= huffman.encoded_data.size());
  huffman.encoded_data_bits = encoded_data_bits;
  return huffman;
}

Huffman Huffman::fromDump(const uint8_t* data, uint8_t size) {
  // 2 bytes for encoded data size in bits
  // 1 byte for tree ch size
  // each tree ch: 1 byte for code length (1..8) and ch count (1..32) + (ch_count * 11 + 7) / 8 bytes for tree (11 bits per ch_count and padding)
  // (encoded_data_bits + 7) / 8 bytes of encoded data
  assert(size >= 3);
  Huffman huffman;
  uint8_t i = 0;
  uint16_t encoded_data_bits;
  std::copy(data, data + sizeof(Huffman::encoded_data_bits), reinterpret_cast<uint8_t*>(&encoded_data_bits));
  assert(encoded_data_bits <= huffman.encoded_data.size());
  const uint16_t encoded_data_size = divide_roundup<uint16_t>(encoded_data_bits, 8u);
  i += sizeof(Huffman::encoded_data_bits);
  const uint8_t tree_data_size = data[i++];
  assert(3 + tree_data_size + encoded_data_size <= size);
  while (i - 3 < tree_data_size) {
    uint8_t ch_info = data[i++];
    uint8_t ch_length = (ch_info >> 5) + 1;
    uint8_t ch_count = (ch_info & 31) + 1;
    unpack11bit(data + i, huffman.tree_data[ch_length], ch_count);
    i += divide_roundup(static_cast<unsigned>(ch_count) * 11u, 8u);
  }
  assert(i - 3 == tree_data_size);
  const std::unordered_map<int16_t, std::pair<uint8_t, std::bitset<8>>> canonical_tree = generateCanonicalTree(huffman.tree_data);
  huffman.encoded_data_bits = encoded_data_bits;
  for (uint16_t j = 0; j < encoded_data_size * 8; j += 8) {
    std::bitset<8> tmp = data[i++];
    for (uint16_t jj = 0; jj < 8; jj++) {
      huffman.encoded_data.set(j + jj, tmp.test(jj));
    }
  }
  decodeFromTreeData(huffman.data, huffman.encoded_data, encoded_data_bits, huffman.tree_data);
  return huffman;
}

void Huffman::dump(uint8_t*& res_data, uint8_t& res_size) const {
  assert(encoded_data_bits <= encoded_data.size());
  const uint16_t encoded_data_size = divide_roundup<uint16_t>(encoded_data_bits, 8u);
  res_size = 3 + encoded_data_size;
  // figure out tree code size
  for (const auto& it : tree_data) {
    uint8_t ch_count = it.second.size();
    assert(ch_count <= 64);
    if (ch_count <= 32) {
      res_size += 1 + divide_roundup(static_cast<unsigned>(ch_count) * 11u, 8u);
    } else {
      // more than 64 is not possible
      res_size += 2 + 44 + divide_roundup(static_cast<unsigned>(ch_count - 32u) * 11u, 8u);
    }
  }
  // begin
  res_data = new uint8_t[res_size];
  uint8_t i = 0;
  reinterpret_cast<uint16_t*>(res_data)[0] = encoded_data_bits;
  i += 2;
  res_data[i++] = res_size - 3 - encoded_data_size;
  for (const auto& it : tree_data) {
    const std::set<int16_t>& set = it.second;
    assert(set.size() <= 64);
    uint8_t ch_count = set.size();
    const uint8_t ch_length = it.first;
    assert(ch_length <= 7);
    auto set_iter = set.begin();
tmp_label:
    const uint8_t _ch_count = std::min<uint8_t>(ch_count, 32u);
    res_data[i++] = ((ch_length - 1) << 5) | (_ch_count - 1);
    pack11bit(res_data + i, set_iter, _ch_count);
    i += divide_roundup(static_cast<unsigned>(_ch_count) * 11u, 8u);
    if (ch_count > 32) {
      ch_count -= 32;
      goto tmp_label;
    }
  }
  assert(i - 3 == res_size - 3 - encoded_data_size);
  // pack encoded data
  for (uint16_t j = 0; j < encoded_data_size * 8; j += 8) {
    std::bitset<8> tmp;
    for (uint16_t jj = 0; jj < 8; jj++) {
      tmp.set(jj, encoded_data.test(j + jj));
    }
    res_data[i++] = static_cast<uint8_t>(tmp.to_ulong());
  }
}

bool Huffman::operator==(const Huffman& huffman) const noexcept {
  assert(encoded_data_bits <= encoded_data.size());
  assert(huffman.encoded_data_bits <= huffman.encoded_data.size());
  if (encoded_data_bits != huffman.encoded_data_bits) {
    return false;
  }
  if (!std::equal(data, data + 64, huffman.data)) {
    return false;
  }
  const auto mask = ~std::bitset<512>() >> (encoded_data_bits); // constexpr since C++23
  assert(mask.size() == encoded_data.size()); // just in case
  if ((encoded_data & mask) != (huffman.encoded_data & mask)) {
    return false;
  }
  return tree_data == huffman.tree_data;
}

bool Huffman::operator!=(const Huffman& huffman) const noexcept {
  return !operator==(huffman);
}
