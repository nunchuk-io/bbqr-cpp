#include <bbqr/bbqr.hpp>
#include <format>
#include <iostream>
#include <string>

using namespace bbqr;

int main(int argc, char** argv) {
  std::string raw = "Nunchuk";
  FileType file_type = FileType::U;

  SplitResult result = split_qrs(raw, file_type);
  // or passing custom option
  SplitResult result2 = split_qrs(raw, file_type, SplitOption{
                                                      .encoding = Encoding::Z,
                                                      .force_encoding = true,
                                                      .min_version = 1,
                                                      .max_version = 40,
                                                      .min_split = 1,
                                                      .max_split = 1295,
                                                  });

  std::cout << "encoding: " << static_cast<char>(result.encoding) << "\n";
  std::cout << "qr version: " << result.version << "\n";
  for (const std::string& part : result.parts) {
    std::cout << part << "\n";
  }
}
