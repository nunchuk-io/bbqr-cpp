#ifndef FILE_UTILS_HPP
#define FILE_UTILS_HPP

#include <algorithm>
#include <cassert>
#include <cctype>
#include <climits>
#include <fstream>
#include <random>
#include <string>
#include <vector>
inline std::vector<unsigned char> read_all_file(const std::string &file_path) {
  std::ifstream t(file_path, std::ios::binary | std::ios::ate);
  std::streamsize size = t.tellg();
  std::vector<unsigned char> buffer(size);
  t.seekg(0, std::ios::beg);
  t.read(reinterpret_cast<char *>(buffer.data()), size);
  return buffer;
}

inline std::vector<std::string> read_lines(const std::string &file_path) {
  std::ifstream t(file_path);
  std::vector<std::string> result;
  std::string buff;
  while (std::getline(t, buff)) {
    if (!buff.empty())
      result.emplace_back(std::move(buff));
  }
  return result;
}

inline std::vector<unsigned char> random_bytes(size_t size) {
  using random_bytes_engine = std::independent_bits_engine<
      std::default_random_engine, CHAR_BIT, unsigned char>;
  random_bytes_engine rbe;
  std::vector<unsigned char> data(size);
  std::generate(begin(data), end(data), std::ref(rbe));
  return data;
}

#endif
