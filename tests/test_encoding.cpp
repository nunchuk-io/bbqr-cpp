#include <bbqr/bbqr.hpp>
#include <bbqr/utils.hpp>
#include <format>

#include "doctest.h"
#include "stringification.h"
#include "test_utils.hpp"

using namespace bbqr;

std::vector<std::string> file_names = {
    "./test_data/1in1000out.psbt",
    "./test_data/1in100out.psbt",
    "./test_data/1in10out.psbt",
    "./test_data/1in20out.psbt",
    "./test_data/1in2out.psbt",
    "./test_data/devils-txn.txn",
    "./test_data/finalized-by-ckcc.txn",
    "./test_data/last.txn",
    "./test_data/nfc-result.txn",
    "./test_data/signed.txn",
};

TEST_CASE("test compression") {
  for (auto &&fname : file_names) {
    auto raw = read_all_file(fname);
    auto [encoded, encoding] = encode_data(raw, Encoding::Z);
    auto decoded = decode_data({encoded}, Encoding::Z);
    auto raw_zip = decode_data({encoded}, Encoding::Base32);
    CHECK(raw == decoded);
    float ratio = 100 - (raw_zip.size() * 100.0 / raw.size());
    MESSAGE(std::format("{}: {} => {} bytes, raito: {}% compression", fname,
                        raw.size(), raw_zip.size(), ratio));
  }
}

TEST_CASE("test split and join") {
  for (auto &&fname : file_names) {
    auto raw = read_all_file(fname);
    SplitOption option{};
    FileType file_type = FileType::B;
    if (std::string_view(fname).ends_with("psbt")) {
      file_type = FileType::P;
    } else if (std::string_view(fname).ends_with("txn")) {
      file_type = FileType::T;
    }
    auto split_result = split_qrs(raw, file_type, option);
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
    CHECK(join_result.raw == raw);
  }
}

TEST_CASE("test base36") {
  for (int i = 0; i < 36 * 36; ++i) {
    std::string b36 = int2base36(i);
    int v = std::stoi(b36, nullptr, 36);
    CHECK(v == i);
  }
  CHECK_THROWS(int2base36(-1));
  CHECK_THROWS(int2base36(36 * 36));
}
