#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_MAKE_DEMO_BOX
#define ADASDF_CL_MAKE_DEMO_BOX ""
#endif
#ifndef ADASDF_CL_WORLD_BROADPHASE
#define ADASDF_CL_WORLD_BROADPHASE ""
#endif
#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

namespace {

std::string executableCommand(const std::string& tool) {
#ifdef _WIN32
  if (tool.find('\\') == std::string::npos && tool.find('/') == std::string::npos) {
    return ".\\" + tool;
  }
#else
  if (tool.find('/') == std::string::npos && tool.find('\\') == std::string::npos) {
    return "./" + tool;
  }
#endif
  return "\"" + tool + "\"";
}

std::string quote(const std::filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

bool makeScene(const std::filesystem::path& scene, const std::filesystem::path& model) {
  std::ofstream file(scene);
  if (!file) {
    return false;
  }
  file << "object_id,name,model_path,samples_path,tx,ty,tz,qw,qx,qy,qz,group,mask,type,enabled\n";
  file << "0,box_a," << model.string() << ",,0,0,0,1,0,0,0,0x1,0xffffffff,dynamic,true\n";
  file << "1,box_b," << model.string() << ",,0,0,0,1,0,0,0,0x2,0xffffffff,dynamic,true\n";
  file << "2,far_box," << model.string() << ",,4,0,0,1,0,0,0,0x4,0xffffffff,dynamic,true\n";
  return true;
}

}  // namespace

int main() {
  const std::string build_tool = ADASDF_CL_MAKE_DEMO_BOX;
  const std::string broadphase_tool = ADASDF_CL_WORLD_BROADPHASE;
  if (build_tool.empty() || broadphase_tool.empty()) {
    std::cout << "SKIP: world broadphase CLI tools were not built\n";
    return 0;
  }
  const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "world_broadphase_cli_dense.sdfbin";
  const auto scene = temp / "world_broadphase_cli_scene.csv";
  const auto out_csv = temp / "world_broadphase_pairs.csv";
  const auto report = temp / "world_broadphase_report.md";
  const auto stdout_txt = temp / "world_broadphase_stdout.txt";
  if (std::system((executableCommand(build_tool) + " " + quote(model) +
                   " --half-extent 0.5 0.5 0.5 > " +
                   quote(temp / "world_broadphase_build.txt"))
                      .c_str()) != 0 ||
      !makeScene(scene, model)) {
    return 1;
  }
  const std::string command =
      executableCommand(broadphase_tool) + " " + quote(scene) +
      " --out " + quote(out_csv) +
      " --report " + quote(report) + " > " + quote(stdout_txt);
  if (std::system(command.c_str()) != 0 ||
      readFile(stdout_txt).find("Overlap pairs: 1") == std::string::npos ||
      readFile(out_csv).find("box_a") == std::string::npos ||
      !std::filesystem::exists(report)) {
    std::cerr << "world broadphase CLI failed\n";
    return 1;
  }
  std::cout << "world broadphase CLI passed\n";
  return 0;
}
