#include <adasdf/adasdf.h>

#include <filesystem>
#include <iostream>
#include <map>
#include <string>

namespace {

void usage() {
  std::cout
      << "Usage: adasdf_write_manifest --case-id case001 --tool tool_name "
         "--input input_path --output output_path --out manifest.json "
         "[--param key=value]\n";
}

bool hasValue(int index, int argc) {
  return index + 1 < argc;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 1) {
    usage();
    return 0;
  }

  std::string case_id = "default";
  std::string tool_name = "unknown";
  std::filesystem::path input_path;
  std::filesystem::path output_path;
  std::filesystem::path out_path;
  std::map<std::string, std::string> parameters =
      adasdf::commandLineParameters(argc, argv);
  const auto timer = adasdf::startStrictRunTimer();

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      usage();
      return 0;
    } else if (arg == "--case-id" && hasValue(i, argc)) {
      case_id = argv[++i];
    } else if (arg == "--tool" && hasValue(i, argc)) {
      tool_name = argv[++i];
    } else if (arg == "--input" && hasValue(i, argc)) {
      input_path = argv[++i];
    } else if (arg == "--output" && hasValue(i, argc)) {
      output_path = argv[++i];
    } else if (arg == "--out" && hasValue(i, argc)) {
      out_path = argv[++i];
    } else if (arg == "--param" && hasValue(i, argc)) {
      const std::string value = argv[++i];
      const std::size_t equal = value.find('=');
      if (equal == std::string::npos) {
        std::cerr << "adasdf_write_manifest: --param expects key=value\n";
        return 1;
      }
      parameters[value.substr(0, equal)] = value.substr(equal + 1);
    } else {
      std::cerr << "Unknown or incomplete option: " << arg << "\n";
      usage();
      return 1;
    }
  }

  if (out_path.empty()) {
    std::cerr << "adasdf_write_manifest: --out is required\n";
    usage();
    return 1;
  }

  std::string error;
  if (!adasdf::writeStrictRunReport(
          out_path,
          tool_name,
          case_id,
          input_path,
          output_path,
          parameters,
          {},
          true,
          "ok",
          "",
          timer,
          &error)) {
    std::cerr << "adasdf_write_manifest: failed to write manifest: "
              << error << "\n";
    return 1;
  }
  std::cout << "Strict manifest: " << out_path.string() << "\n";
  return 0;
}

