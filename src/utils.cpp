#include "bbqr/utils.hpp"

#include "strencoding.hpp"
#include "zlib.h"

namespace bbqr {
static std::string zlib_compress(std::string_view source) {
  std::string buff(source.size(), '\0');

  z_stream stream;
  stream.zalloc = (alloc_func)0;
  stream.zfree = (free_func)0;
  stream.opaque = (voidpf)0;
  stream.next_out = (Bytef *)(buff.data());
  stream.avail_out = buff.size();
  stream.next_in = (Bytef *)(source.data());
  stream.avail_in = source.size();

  int ret = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -10, 8,
                         Z_DEFAULT_STRATEGY);
  if (ret != Z_OK)
    throw std::runtime_error("deflateInit failed: " + std::to_string(ret));

  while (true) {
    ret = deflate(&stream, Z_FINISH);
    if (ret == Z_STREAM_END) {
      break;
    } else if (ret == Z_OK || ret == Z_BUF_ERROR) {
      auto size = buff.size();
      buff.resize(size * 2);
      stream.next_out = (Bytef *)(buff.data() + stream.total_out);
      stream.avail_out = buff.size() - size;
    } else {
      throw std::runtime_error("deflate failed: " + std::to_string(ret));
    }
  }

  deflateEnd(&stream);
  buff.resize(stream.total_out);
  return buff;
}

template <typename RawType>
static RawType zlib_uncompress(const std::vector<unsigned char> &source) {
  static constexpr int INITIAL_BUFFER_SIZE = 1024;
  RawType buff(INITIAL_BUFFER_SIZE, '\0');

  z_stream stream;
  stream.zalloc = (alloc_func)0;
  stream.zfree = (free_func)0;
  stream.opaque = (voidpf)0;
  stream.next_out = (Bytef *)(buff.data());
  stream.avail_out = buff.size();
  stream.next_in = (Bytef *)(source.data());
  stream.avail_in = source.size();

  int ret = inflateInit2(&stream, -10);
  if (ret != Z_OK)
    throw std::runtime_error("inflateInit failed: " + std::to_string(ret));

  while (true) {
    ret = inflate(&stream, Z_NO_FLUSH);
    if (ret == Z_STREAM_END) {
      break;
    } else if (ret == Z_BUF_ERROR) {
      auto size = buff.size();
      buff.resize(size * 2);
      stream.next_out = (Bytef *)(buff.data() + stream.total_out);
      stream.avail_out = buff.size() - size;
    } else if (ret != Z_OK) {
      throw std::runtime_error("inflate failed: " + std::to_string(ret));
    }
  };

  inflateEnd(&stream);
  buff.resize(stream.total_out);
  return buff;
}

std::pair<std::string, Encoding> encode_data(std::string_view raw, Encoding encoding, bool force_encoding) {
  switch (encoding) {
    case Encoding::H:
      return {HexStr(raw), encoding};
    case Encoding::Base32:
      return {EncodeBase32(raw), encoding};
    case Encoding::Z: {
      auto compressed = zlib_compress(raw);
      bool use_z = compressed.size() < raw.size() || force_encoding;
      return use_z ? std::make_pair(EncodeBase32(compressed), Encoding::Z)
                   : std::make_pair(EncodeBase32(raw), Encoding::Base32);
    }
  }
  throw std::invalid_argument("Invalid encoding");
}

std::pair<std::string, Encoding> encode_data(const std::vector<unsigned char> &raw, Encoding encoding, bool force_encoding) {
  return encode_data(std::string_view(reinterpret_cast<const char *>(raw.data()), raw.size()), encoding, force_encoding);
}

template <typename RawType, typename Container>
RawType decode_data_c(const Container &parts, Encoding encoding) {
  switch (encoding) {
    case Encoding::H: {
      RawType result;
      for (auto &&part : parts) {
        auto v = TryParseHex(part);
        if (!v) {
          throw std::invalid_argument("Invalid hex");
        }
        result.insert(result.end(), v->begin(), v->end());
      }
      return result;
    }
    case Encoding::Base32: {
      RawType result;
      for (auto &&part : parts) {
        auto v = DecodeBase32(part);
        if (!v) {
          throw std::invalid_argument("Invalid base32");
        }
        result.insert(result.end(), v->begin(), v->end());
      }
      return result;
    }
    case Encoding::Z: {
      std::vector<unsigned char> buff;
      for (auto &&part : parts) {
        auto v = DecodeBase32(part);
        if (!v) {
          throw std::invalid_argument("Invalid base32");
        }
        buff.insert(buff.end(), v->begin(), v->end());
      }

      return zlib_uncompress<RawType>(buff);
    }
  }
  throw std::invalid_argument("Invalid encoding");
}

template <typename RawType>
RawType decode_data(const std::vector<std::string_view> &parts, Encoding encoding) {
  return decode_data_c<RawType>(parts, encoding);
}

template <typename RawType>
RawType decode_data(const std::vector<std::string> &parts, Encoding encoding) {
  return decode_data_c<RawType>(parts, encoding);
}

template <typename RawType>
RawType decode_data(const std::initializer_list<std::string> &parts, Encoding encoding) {
  return decode_data_c<RawType>(parts, encoding);
}

template std::string decode_data(const std::vector<std::string_view> &parts, Encoding encoding);
template std::vector<unsigned char> decode_data(const std::vector<std::string_view> &parts, Encoding encoding);
template std::string decode_data(const std::vector<std::string> &parts, Encoding encoding);
template std::vector<unsigned char> decode_data(const std::vector<std::string> &parts, Encoding encoding);
template std::string decode_data(const std::initializer_list<std::string> &parts, Encoding encoding);
template std::vector<unsigned char> decode_data(const std::initializer_list<std::string> &parts, Encoding encoding);

std::string int2base36(int num) {
  if (num < 0 || num > 1295) {
    throw std::out_of_range("Out of range");
  }

  constexpr auto tostr = [](int x) -> char {
    return (x < 10) ? '0' + x : 'A' + x - 10;
  };

  std::string ret({tostr(num / 36), tostr(num % 36)});
  return ret;
}

}  // namespace bbqr
