#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const std::filesystem::path file = temp / "sha256_abc.txt";
  {
    std::ofstream out(file, std::ios::binary);
    out << "abc";
  }
  const std::string digest = adasdf::fileSha256(file);
  if (digest !=
      "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") {
    std::cerr << "SHA-256 mismatch: " << digest << "\n";
    return 1;
  }
  const adasdf::RunManifest manifest =
      adasdf::makeRunManifest("tool", "case", file, {});
  if (manifest.adasdf_version != adasdf::versionString() ||
      manifest.schema_version != adasdf::strictReportSchemaVersion() ||
      manifest.input_sha256 != digest) {
    std::cerr << "run manifest fields are wrong\n";
    return 1;
  }
  std::cout << "run manifest passed\n";
  return 0;
}

