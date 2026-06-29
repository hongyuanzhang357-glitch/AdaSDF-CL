#include <adasdf/adasdf.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#ifndef ADASDF_CL_MAKE_DEMO_BOX
#define ADASDF_CL_MAKE_DEMO_BOX ""
#endif

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string executableCommand(const std::string& tool) {
  const std::filesystem::path path(tool);
  if (path.has_parent_path()) {
    return tool;
  }
#ifdef _WIN32
  return ".\\" + tool;
#else
  return "./" + tool;
#endif
}

}  // namespace

int main() {
  try {
    const std::string tool = ADASDF_CL_MAKE_DEMO_BOX;
    if (tool.empty() || !std::filesystem::exists(tool)) {
      std::cerr << "adasdf_make_demo_box executable is missing: " << tool << "\n";
      return 1;
    }
    const std::string command_tool = executableCommand(tool);

    if (std::system(command_tool.c_str()) != 0) {
      std::cerr << "adasdf_make_demo_box usage invocation failed\n";
      return 1;
    }

    std::filesystem::path dir = "adasdf_cl_test_tmp";
    std::filesystem::create_directories(dir);
    const std::filesystem::path output = dir / "cli_demo_box.sdfbin";

    const std::string command =
        command_tool + " " + output.string() +
        " --center 0 0 0 --half-extent 0.5 0.5 0.5";
    if (std::system(command.c_str()) != 0) {
      std::cerr << "adasdf_make_demo_box generation failed\n";
      return 1;
    }

    auto model = adasdf::SDFBinReader::read(output);
    if (!model || !model->isValid() || !model->queryBackendAvailable()) {
      std::cerr << "CLI generated demo sdfbin could not be read\n";
      return 1;
    }
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_make_demo_box_cli failed: " << exc.what() << "\n";
    return 1;
  }
}
