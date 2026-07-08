#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <iostream>
#include <string>

#include "ModelJsonHelpers.h"

namespace {

void usage() {
  std::cout << "Usage: adasdf_export_structure model.sdfbin [--json]\n";
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
        std::cerr << "adasdf_export_structure: unknown option: " << arg
                  << "\n";
        return 2;
      } else if (path.empty()) {
        path = arg;
      } else {
        std::cerr << "adasdf_export_structure: unexpected argument: " << arg
                  << "\n";
        return 2;
      }
    }
    if (path.empty() || !std::filesystem::exists(path)) {
      std::cerr << "adasdf_export_structure: model missing: " << path.string()
                << "\n";
      return 2;
    }
    const auto model = adasdf::SDFBinReader::read(path);
    if (!model || !model->isValid()) {
      std::cerr << "adasdf_export_structure: failed to load model\n";
      return 1;
    }
    if (!json) {
      std::cout << "Model type: " << adasdf_tools::modelType(*model) << "\n";
      std::cout << "Block count: " << adasdf_tools::blockCount(*model)
                << "\n";
      return 0;
    }
    adasdf::BackendJsonContract contract = adasdf_tools::makeBaseContract(
        adasdf::SchemaIds::Structure,
        "adasdf_export_structure");
    contract.payload_fields.push_back(
        {"model_type",
         adasdf::JsonContractWriter::quote(adasdf_tools::modelType(*model))});
    contract.payload_fields.push_back(
        {"bounds", adasdf::JsonContractWriter::aabb(model->boundingBox())});
    contract.payload_fields.push_back(
        {"blocks", adasdf_tools::blocksJson(*model)});
    contract.payload_fields.push_back(
        {"levels", adasdf_tools::levelsJson(*model)});
    contract.payload_fields.push_back(
        {"hierarchy", "{\"source\":\"runtime_model_metadata\"}"});
    std::cout << adasdf::JsonContractWriter::writeObject(contract);
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "adasdf_export_structure failed: " << exc.what() << "\n";
    return 1;
  }
}
