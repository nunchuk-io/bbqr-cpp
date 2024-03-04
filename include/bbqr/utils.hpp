#ifndef UTILS_HPP
#define UTILS_HPP

#include <initializer_list>
#include <string>

#include "bbqr/bbqr.hpp"

namespace bbqr {
std::pair<std::string, Encoding> encode_data(std::string_view raw, Encoding encoding, bool force_encoding = false);
std::pair<std::string, Encoding> encode_data(const std::vector<unsigned char> &raw, Encoding encoding, bool force_encoding = false);

template <typename RawType = std::vector<unsigned char>>
RawType decode_data(const std::vector<std::string_view> &parts, Encoding encoding);
template <typename RawType = std::vector<unsigned char>>
RawType decode_data(const std::vector<std::string> &parts, Encoding encoding);
template <typename RawType = std::vector<unsigned char>>
RawType decode_data(const std::initializer_list<std::string> &parts, Encoding encoding);

std::string int2base36(int num);
}  // namespace bbqr

#endif
