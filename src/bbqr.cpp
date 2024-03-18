#include "bbqr/bbqr.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>

#include "bbqr/utils.hpp"

constexpr int HEADER_LEN = 8;

namespace bbqr {

static int version_to_chars(int version) {
  // return number of * *chars * *that fit into indicated version QR
  //- assumes L for ECC
  //- assumes alnum encoding
  static constexpr int QR_DATA_CAPACITY[41] = {
      0, 25, 47, 77, 114, 154, 195, 224, 279, 335, 395,
      468, 535, 619, 667, 758, 854, 938, 1046, 1153, 1249, 1352,
      1460, 1588, 1704, 1853, 1990, 2132, 2223, 2369, 2520, 2677, 2840,
      3009, 3183, 3351, 3537, 3729, 3927, 4087, 4296};
  return QR_DATA_CAPACITY[version];
}

static std::pair<int /* count */, int /* per each */>
num_qr_needed(int version, int size, int split_mod) {
  int baseCap = version_to_chars(version) - HEADER_LEN;
  int adjustedCap = baseCap - (baseCap % split_mod);
  int estimatedCount = (size + adjustedCap - 1) / adjustedCap;

  if (estimatedCount == 1) {
    return {1, size};
  }

  int estimatedCap = ((estimatedCount - 1) * adjustedCap) + baseCap;
  int count = (estimatedCap >= size) ? estimatedCount : estimatedCount + 1;
  return {count, adjustedCap};
}

static std::tuple<int /* count */, int /* version */, int /* per each */>
find_best_version(int size, int split_mod, int min_split, int max_split,
                  int min_version, int max_version) {
  std::vector<std::tuple<int, int, int>> options;
  for (int version = min_version; version <= max_version; ++version) {
    auto [count, per_each] = num_qr_needed(version, size, split_mod);
    if (min_split <= count && count <= max_split) {
      options.emplace_back(count, version, per_each);
    }
  }
  if (options.empty()) {
    throw std::invalid_argument("Cannot make it fit");
  }
  std::sort(options.begin(), options.end());
  return options.front();
}

static int get_split_mod(Encoding encoding) {
  return encoding == Encoding::H ? 2 : 8;
}

static void validateSplitOption(const SplitOption& option) {
  constexpr auto is_valid_version = [](int version) {
    return version >= 1 && version <= 40;
  };

  constexpr auto is_valid_split = [](int split) {
    return split >= 1 && split <= 1295;
  };

  if (option.min_version > option.max_version ||
      !is_valid_version(option.min_version) ||
      !is_valid_version(option.max_version)) {
    throw std::out_of_range("min/max version out of range");
  }

  if (option.min_split > option.max_split ||
      !is_valid_split(option.min_split) ||
      !is_valid_split(option.max_split)) {
    throw std::out_of_range("min/max split out of range");
  }
}

SplitResult split_qrs(std::string_view raw, FileType file_type, const SplitOption& option) {
  validateSplitOption(option);

  auto [encoded, encoding] = encode_data(raw, option.encoding, option.force_encoding);
  int size = encoded.size();

  auto [count, version, per_each] = find_best_version(
      size, get_split_mod(encoding), option.min_split, option.max_split,
      option.min_version, option.max_version);

  std::vector<std::string> parts;
  parts.reserve(count);

  for (int offset = 0, i = 0; offset < size; offset += per_each, ++i) {
    std::string buff;
    buff.reserve(HEADER_LEN + per_each);
    buff.append("B$");
    buff.push_back(static_cast<char>(encoding));
    buff.push_back(static_cast<char>(file_type));
    buff.append(int2base36(count));
    buff.append(int2base36(i));
    buff.append(encoded.data() + offset, std::min(per_each, size - offset));
    parts.emplace_back(std::move(buff));
  }
  return SplitResult{
      .version = version,
      .parts = std::move(parts),
      .encoding = encoding,
  };
}

SplitResult split_qrs(const std::vector<unsigned char>& raw, FileType file_type, const SplitOption& option) {
  return split_qrs(std::string_view(reinterpret_cast<const char*>(raw.data()), raw.size()), file_type, option);
}

template <typename RawType>
JoinResult<RawType> join_qrs(const std::vector<std::string>& parts) {
  std::string_view header;
  for (auto&& part : parts) {
    if (part.size() <= HEADER_LEN) {
      throw std::invalid_argument("Invalid header data");
    }
    if (header.empty()) {
      header = std::string_view(part.data(), 6);
    } else if (std::string_view(part.data(), 6) != header) {
      throw std::invalid_argument("conflicting/variable filetype/encodings/sizes");
    }
  }
  if (header.substr(0, 2) != "B$") {
    throw std::invalid_argument("fixed header not found, expected B$");
  }

  Encoding encoding = static_cast<Encoding>(header[2]);
  FileType file_type = static_cast<FileType>(header[3]);
  size_t count = std::stoi(std::string(header.substr(4, 2)), nullptr, 36);

  if (count == 0) {
    throw std::invalid_argument("Invalid QR");
  }

  std::vector<std::pair<int, std::string_view>> data;
  data.reserve(parts.size());

  for (auto&& part : parts) {
    size_t idx = std::stoi(part.substr(6, 2), nullptr, 36);
    if (idx >= count) {
      throw std::invalid_argument("got part " + std::to_string(idx) + " but only expecting " + std::to_string(count));
    }
    data.emplace_back(idx, std::string_view(part.data() + 8, part.data() + part.size()));
  }

  std::sort(data.begin(), data.end());
  data.erase(std::unique(data.begin(), data.end()), data.end());

  if (data.size() == count) {
    std::vector<std::string_view> encoded(data.size());
    std::transform(data.begin(), data.end(), encoded.begin(),
                   [](const std::pair<int, std::string_view>& p) { return p.second; });
    auto decoded = decode_data<RawType>(encoded, encoding);
    return JoinResult<RawType>{
        .file_type = file_type,
        .encoding = encoding,
        .raw = std::move(decoded),
        .expected_part_count = count,
        .processed_parts_count = count,
        .is_complete = true,
    };
  }

  if (data.size() < count) {
    return JoinResult<RawType>{
        .file_type = file_type,
        .encoding = encoding,
        .raw = (RawType{}),
        .expected_part_count = count,
        .processed_parts_count = data.size(),
        .is_complete = false,
    };
  }

  throw std::invalid_argument("Duplicate part has wrong content");
}

template JoinResult<std::vector<unsigned char>> join_qrs(const std::vector<std::string>& parts);
template JoinResult<std::string> join_qrs(const std::vector<std::string>& parts);

}  // namespace bbqr
