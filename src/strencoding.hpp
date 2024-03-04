#ifndef STRENCODING_HPP
#define STRENCODING_HPP

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

namespace {
/** Helper class for the default infn argument to ConvertBits (just returns the input). */
struct IntIdentity {
  [[maybe_unused]] int operator()(int x) const { return x; }
};

using ByteAsHex = std::array<char, 2>;

constexpr std::array<ByteAsHex, 256> CreateByteToHexMap() {
  constexpr char hexmap[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  std::array<ByteAsHex, 256> byte_to_hex{};
  for (size_t i = 0; i < byte_to_hex.size(); ++i) {
    byte_to_hex[i][0] = hexmap[i >> 4];
    byte_to_hex[i][1] = hexmap[i & 15];
  }
  return byte_to_hex;
}

}  // namespace

/** Convert from one power-of-2 number base to another. */
template <int frombits, int tobits, bool pad, typename O, typename It, typename I = IntIdentity>
bool ConvertBits(O outfn, It it, It end, I infn = {}) {
  size_t acc = 0;
  size_t bits = 0;
  constexpr size_t maxv = (1 << tobits) - 1;
  constexpr size_t max_acc = (1 << (frombits + tobits - 1)) - 1;
  while (it != end) {
    int v = infn(*it);
    if (v < 0)
      return false;
    acc = ((acc << frombits) | v) & max_acc;
    bits += frombits;
    while (bits >= tobits) {
      bits -= tobits;
      outfn((acc >> bits) & maxv);
    }
    ++it;
  }
  if (pad) {
    if (bits)
      outfn((acc << (tobits - bits)) & maxv);
  } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
    return false;
  }
  return true;
}

inline std::string EncodeBase32(std::string_view input, bool pad = false) {
  static const char* pbase32 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

  std::string str;
  str.reserve(((input.size() + 4) / 5) * 8);
  ConvertBits<8, 5, true>([&](int v) { str += pbase32[v]; }, input.begin(),
                          input.end(), [](char c) -> unsigned char { return c; });
  if (pad) {
    while (str.size() % 8) {
      str += '=';
    }
  }
  return str;
}

inline std::optional<std::vector<unsigned char>> DecodeBase32(std::string_view str) {
  static constexpr int8_t decode32_table[256]{
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
      15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 0, 1, 2,
      3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
      23, 24, 25, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  // if (str.size() % 8 != 0)
  //   return {};
  /* 1, 3, 4, or 6 padding '=' suffix characters are permitted. */
  // if (str.size() >= 1 && str.back() == '=')
  //   str.remove_suffix(1);
  // if (str.size() >= 2 && str.substr(str.size() - 2) == "==")
  //   str.remove_suffix(2);
  // if (str.size() >= 1 && str.back() == '=')
  //   str.remove_suffix(1);
  // if (str.size() >= 2 && str.substr(str.size() - 2) == "==")
  //   str.remove_suffix(2);

  while (!str.empty() && str.back() == '=') {
    str.remove_suffix(1);
  }
  std::vector<unsigned char> ret;
  ret.reserve((str.size() * 5) / 8);
  bool valid = ConvertBits<5, 8, false>(
      [&](unsigned char c) { ret.push_back(c); },
      str.begin(), str.end(),
      [](char c) { return decode32_table[uint8_t(c)]; });
  if (!valid)
    return {};

  return ret;
}

inline std::string HexStr(const std::string_view str) {
  std::string rv(str.size() * 2, '\0');
  static constexpr auto byte_to_hex = CreateByteToHexMap();
  static_assert(sizeof(byte_to_hex) == 512);

  char* it = rv.data();
  for (uint8_t v : str) {
    std::memcpy(it, byte_to_hex[v].data(), 2);
    it += 2;
  }

  assert(it == rv.data() + rv.size());
  return rv;
}

inline signed char HexDigit(char c) {
  static constexpr signed char p_util_hexdigit[256] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
      -1, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  return p_util_hexdigit[(unsigned char)c];
}

constexpr inline bool IsSpace(char c) noexcept {
  return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

template <typename Byte = uint8_t>
inline std::optional<std::vector<Byte>> TryParseHex(const std::string_view str) {
  std::vector<Byte> vch;
  auto it = str.begin();
  vch.reserve(str.size() / 2);
  while (it != str.end()) {
    if (IsSpace(*it)) {
      ++it;
      continue;
    }
    auto c1 = HexDigit(*(it++));
    if (it == str.end())
      return std::nullopt;
    auto c2 = HexDigit(*(it++));
    if (c1 < 0 || c2 < 0)
      return std::nullopt;
    vch.push_back(Byte(c1 << 4) | Byte(c2));
  }
  return vch;
}

#endif
