#include <bbqr/bbqr.hpp>

#include "doctest.h"
#include "test_utils.hpp"

using namespace bbqr;

TEST_CASE("test real scan") {
  auto lines = read_lines("./test_data/real-scan.txt");

  auto j = join_qrs<std::string>(lines);
  CHECK(!j.raw.empty());
  CHECK(j.raw.find_first_of("Zlib compressed") != std::string::npos);
  CHECK(j.raw.find_first_of("PSBT") != std::string::npos);
}
