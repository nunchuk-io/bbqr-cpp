#include <bbqr/bbqr.hpp>
#include <bbqr/utils.hpp>
#include <cstdlib>
#include <format>
#include <numeric>
#include <ranges>

#include "doctest.h"
#include "stringification.h"
#include "test_utils.hpp"

using namespace bbqr;

TEST_CASE("test loopback") {
  FileType file_type = FileType::P;
  std::vector<Encoding> encodings = {Encoding::H, Encoding::Base32, Encoding::Z};
  std::vector<int> sizes = {10, 100, 2000, 10'000, 50'000};
  std::vector<int> max_versions = {11, 29, 40};
  std::vector<bool> low_ents = {true, false};

  for (const auto &[encoding, size, max_version, low_ent] : std::ranges::views::cartesian_product(encodings, sizes, max_versions, low_ents)) {
    std::vector<unsigned char> data = low_ent ? std::vector<unsigned char>(size, 'A') : random_bytes(size);
    SplitOption option{.max_version = max_version};

    auto split_result = split_qrs(data, file_type, option);

    CHECK(split_result.version >= option.min_version);
    CHECK(split_result.version <= option.max_version);
    CHECK(split_result.parts.size() >= option.min_split);
    CHECK(split_result.parts.size() <= option.max_split);
    if (option.encoding != Encoding::Z || option.force_encoding) {
      CHECK(split_result.encoding == option.encoding);
    } else {
      CHECK((split_result.encoding == Encoding::Z || split_result.encoding == Encoding::Base32) == true);
    }

    auto join_result = join_qrs(split_result.parts);
    CHECK(join_result.is_complete == true);
    CHECK(join_result.expected_part_count == split_result.parts.size());
    CHECK(join_result.processed_parts_count == split_result.parts.size());
    CHECK(join_result.file_type == file_type);
    CHECK(join_result.raw == data);
  }
}

TEST_CASE("test min split") {
  std::vector<int> min_splits = {2, 3, 4, 5, 6, 7, 8, 9};
  int size = 10'000;
  FileType file_type = FileType::T;

  for (int min_split : min_splits) {
    SplitOption option{.min_split = min_split};
    std::vector<unsigned char> data = random_bytes(size);
    auto split_result = split_qrs(data, file_type, option);

    CHECK(split_result.parts.size() >= option.min_split);
    auto join_result = join_qrs(split_result.parts);

    CHECK(join_result.file_type == file_type);
    CHECK(join_result.raw == data);
  }
}

TEST_CASE("Version 27 edge cases") {
  FileType file_type = FileType::T;
  std::vector<Encoding> encodings = {Encoding::H, Encoding::Base32, Encoding::Z};
  std::vector<int> sizes(20);
  std::iota(sizes.begin(), sizes.end(), 1060);
  std::vector<bool> low_ents = {true, false};

  for (const auto &[encoding, size, low_ent] : std::ranges::views::cartesian_product(encodings, sizes, low_ents)) {
    int need = 27;
    std::vector<unsigned char> data = low_ent ? std::vector<unsigned char>(size, 'A') : random_bytes(size);
    SplitOption option{
        .encoding = encoding,
        .min_version = need,
        .max_version = need,
        .max_split = 2,
    };

    auto split_result = split_qrs(data, file_type, option);
    CHECK(split_result.version == need);
    // MESSAGE(std::format("count: {} encoding: {} size: {} version {}",
    //                     split_result.parts.size(), static_cast<char>(split_result.encoding), size, split_result.version));

    if (encoding == Encoding::H) {
      if (size <= 1062) {
        CHECK(split_result.parts.size() == 1);
      } else {
        CHECK(split_result.parts.size() == 2);
      }
    } else if (encoding == Encoding::Z) {
      CHECK(split_result.parts.size() == 1);
      if (low_ent) {
        CHECK(split_result.parts[0].size() < 100);
      }
    } else if (encoding == Encoding::Base32) {
      CHECK(split_result.parts.size() == 1);
    }
    auto join_result = join_qrs(split_result.parts);

    CHECK(join_result.file_type == file_type);
    CHECK(join_result.raw == data);
  }
}

TEST_CASE("Test max size") {
  int nc = 4296 - 8;  // version 40 capacity in chars, less header
  int nparts = 36 * 36 - 1;

  std::vector<Encoding> encodings = {Encoding::H, Encoding::Base32};
  FileType file_type = FileType::T;

  for (Encoding encoding : encodings) {
    int pkt_size = encoding == Encoding::H ? nc / 2 : nc * 5 / 8;
    std::vector<unsigned char> data = random_bytes(nparts * pkt_size);

    SplitOption option{
        .encoding = encoding,
        .min_version = 40,
    };

    auto split_result = split_qrs(data, file_type, option);

    CHECK(split_result.version == 40);
    CHECK(split_result.parts.size() == nparts);
    auto join_result = join_qrs(split_result.parts);

    CHECK(join_result.file_type == file_type);
    CHECK(join_result.raw == data);
  }
}
