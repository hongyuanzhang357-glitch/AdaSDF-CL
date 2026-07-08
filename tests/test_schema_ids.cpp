#include <adasdf/adasdf.h>

#include <iostream>
#include <string>

int main() {
  if (std::string(adasdf::SchemaIds::Info) != "adasdf.info.v1" ||
      std::string(adasdf::SchemaIds::Structure) != "adasdf.structure.v1" ||
      std::string(adasdf::SchemaIds::BlockGrid) != "adasdf.block_grid.v1" ||
      std::string(adasdf::SchemaIds::Compression) !=
          "adasdf.compression.v1" ||
      std::string(adasdf::SchemaIds::Collide) != "adasdf.collide.v1" ||
      std::string(adasdf::SchemaIds::Benchmark) != "adasdf.benchmark.v1" ||
      std::string(adasdf::SchemaIds::BuildProfile) !=
          "adasdf.build_profile.v1" ||
      std::string(adasdf::SchemaIds::Error) != "adasdf.error.v1") {
    std::cerr << "schema ids changed\n";
    return 1;
  }
  return 0;
}
