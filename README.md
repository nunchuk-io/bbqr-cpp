# BBQr - Better Bitcoin QR

Encodes larger files into a series of QR codes so they can cross air gaps.

## Specification

See full spec [https://bbqr.org/BBQr.html](https://bbqr.org/BBQr.html).


## Setup

``` bash
$ cd your_project/
$ git submodule add https://github.com/nunchuk-io/bbqr-cpp
$ git submodule update --init --recursive
```

Add the following to your `CMakeLists.txt`.

``` cmake
add_subdirectory(bbqr-cpp)
target_link_libraries("${PROJECT_NAME}" PUBLIC bbqr-cpp)
```

## Usage
To split data into BBQr:
``` cpp
#include <bbqr/bbqr.hpp>
using namespace bbqr;

std::string raw = "your data";
// std::vector<unsigned char> raw = your_binary_data;
FileType file_type = FileType::U; // type of the raw data U=UTF-8

SplitResult split_result = split_qrs(raw, file_type);
// or with custom options
SplitResult split_result = split_qrs(raw, file_type, SplitOption{
                                    .encoding = Encoding::Z, 
                                    .force_encoding = false,
                                    .min_version = 1,
                                    .max_version = 40,
                                    .min_split = 1,
                                    .max_split = 1295
                                    });

split_result.version; // the QR code version chosen for best efficiency
split_result.encoding; // the actual encoding used
split_result.parts; // the QR code parts
```

To join BBQr:
``` cpp
try {
    std::vector<std::string> qrs = split_result.parts;  
    JoinResult join_result = join_qrs<std::string>(qrs); 
    // or join_qrs(qrs) to get std::vector<unsigned char>
    if (join_result.is_complete) {
      // use join_result
      assert(join_result.raw == raw);
      assert(join_result.file_type == file_type);
      join_result.encoding;
    } else {
      // missing QRs
      float progress = join_result.processed_parts_count * 1.0 / join_result.expected_part_count;
    }
  } catch (std::exception& e) {
    // handle exception
  }
```

For more examples see [examples](./examples).

## Contributing

### Build & run tests
```
cd tests
mkdir build
cd build
cmake ..
make all test -j4
```
