#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#include "ModelJsonHelpers.h"

namespace {

void usage() {
  std::cout << "Usage: adasdf_export_compression model.sdfbin [--json]\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 1) {
      usage();
      return 0;
    }
    std::filesystem::path path;
    bool json = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--json") {
        json = true;
      } else if (arg == "--help" || arg == "-h") {
        usage();
        return 0;
      } else if (!arg.empty() && arg[0] == '-') {
        std::cerr << "adasdf_export_compression: unknown option: " << arg
                  << "\n";
        return 2;
      } else if (path.empty()) {
        path = arg;
      } else {
        std::cerr << "adasdf_export_compression: unexpected argument: " << arg
                  << "\n";
        return 2;
      }
    }
    if (path.empty() || !std::filesystem::exists(path)) {
      std::cerr << "adasdf_export_compression: model missing: "
                << path.string() << "\n";
      return 2;
    }
    const auto model = adasdf::SDFBinReader::read(path);
    if (!model || !model->isValid()) {
      std::cerr << "adasdf_export_compression: failed to load model\n";
      return 1;
    }
    if (!json) {
      std::cout << "Compression: "
                << (dynamic_cast<const adasdf::CompressedAdaptiveBlockSDFModel*>(
                        model.get()) != nullptr
                        ? "compressed"
                        : "not-compressed")
                << "\n";
      return 0;
    }
    adasdf::BackendJsonContract contract = adasdf_tools::makeBaseContract(
        adasdf::SchemaIds::Compression,
        "adasdf_export_compression");
    contract.payload_fields.push_back(
        {"model_type",
         adasdf::JsonContractWriter::quote(adasdf_tools::modelType(*model))});
    contract.payload_fields.push_back(
        {"block_counts",
         adasdf_tools::compressionPayloadField(*model, "block_counts")});
    contract.payload_fields.push_back(
        {"rank_stats",
         adasdf_tools::compressionPayloadField(*model, "rank_stats")});
    contract.payload_fields.push_back(
        {"memory", adasdf_tools::compressionPayloadField(*model, "memory")});
    contract.payload_fields.push_back(
        {"compression_ratio",
         adasdf_tools::compressionPayloadField(*model, "compression_ratio")});
    contract.payload_fields.push_back(
        {"error_summary",
         adasdf_tools::compressionPayloadField(*model, "error_summary")});
    std::cout << adasdf::JsonContractWriter::writeObject(contract);
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_export_compression failed: " << exc.what() << "\n";
    return 1;
  }
}
