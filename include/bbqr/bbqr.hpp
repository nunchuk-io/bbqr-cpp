#ifndef BBQR_HPP
#define BBQR_HPP

#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace bbqr {
enum class FileType : char {
  P = 'P',  // PSBT file
  T = 'T',  // Ready to send Bitcoin wire transaction
  J = 'J',  // JSON data (general purpose)
  C = 'C',  // CBOR data (general purpose)
  U = 'U',  // Unicode text (UTF-8 encoded, simple text)
  B = 'B',  // Binary data (generic octet stream)
  X = 'X',  // Executable data (platform dependant)
};

enum class Encoding : char {
  H = 'H',       // HEX (capitalized hex digits, 4-bits each)
  Base32 = '2',  // Base32 using RFC 4648 alphabet
  Z = 'Z',       // Zlib compressed (wbits=-10, no header)
};

struct SplitOption {
  Encoding encoding = Encoding::Z;  // The encoding type (default is Z)
  bool force_encoding = false;      // Whether to force the specified encoding
  int min_version = 5;              // Minimum QR code version for encoding (default is 5)
  int max_version = 40;             // Maximum QR code version for encoding (default is 40)
  int min_split = 1;                // Minimum split size for encoding (default is 1)
  int max_split = 1295;             // Minimum split size for encoding (default is max base36 = 1295)
};

struct SplitResult {
  int version;                     // The QR code version
  std::vector<std::string> parts;  // QR code parts
  Encoding encoding;               // The actual encoding used
};

SplitResult split_qrs(std::string_view raw, FileType file_type, const SplitOption &option = SplitOption());
SplitResult split_qrs(const std::vector<unsigned char> &raw, FileType file_type, const SplitOption &option = SplitOption());

template <typename RawType>
struct JoinResult {
  static_assert(std::is_same<RawType, std::vector<unsigned char>>{} ||
                std::is_same<RawType, std::string>{});

  FileType file_type;
  Encoding encoding;
  RawType raw;

  size_t expected_part_count;
  size_t processed_parts_count;
  bool is_complete;
};

template <typename RawType = std::vector<unsigned char>>
JoinResult<RawType> join_qrs(const std::vector<std::string> &parts);
}  // namespace bbqr

#endif
