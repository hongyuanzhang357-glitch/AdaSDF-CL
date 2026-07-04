#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#ifndef ADASDF_CL_MAKE_DEMO_BOX
#define ADASDF_CL_MAKE_DEMO_BOX ""
#endif
#ifndef ADASDF_CL_BENCHMARK_COLLISION_WORLD
#define ADASDF_CL_BENCHMARK_COLLISION_WORLD ""
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

bool writeFixtureFiles(
    const std::filesystem::path& scene,
    const std::filesystem::path& model,
    const std::filesystem::path& samples) {
  {
    std::ofstream file(samples);
    if (!file) {
      return false;
    }
    file << "id,x,y,z,radius,object_id,link_id,group_id,weight,label\n";
    file << "0,0,0,0,0,0,0,0,1,center\n";
  }
  {
    std::ofstream file(scene);
    if (!file) {
      return false;
    }
    file << "object_id,name,model_path,samples_path,tx,ty,tz,qw,qx,qy,qz,group,mask,type,enabled\n";
    file << "0,box_a," << model.string() << "," << samples.string()
         << ",0,0,0,1,0,0,0,0x1,0xffffffff,dynamic,true\n";
    file << "1,box_b," << model.string() << "," << samples.string()
         << ",0,0,0,1,0,0,0,0x2,0xffffffff,dynamic,true\n";
  }
  return true;
}

}  // namespace

int main() {
  const std::string build_tool = ADASDF_CL_MAKE_DEMO_BOX;
  const std::string bench_tool = ADASDF_CL_BENCHMARK_COLLISION_WORLD;
  if (build_tool.empty() || bench_tool.empty()) {
    std::cout << "SKIP: collision world benchmark CLI was not built\n";
    return 0;
  }
  const std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto model = temp / "world_benchmark_cli_dense.sdfbin";
  const auto samples = temp / "world_benchmark_samples.csv";
  const auto scene = temp / "world_benchmark_scene.csv";
  const auto out_csv = temp / "world_benchmark.csv";
  const auto stdout_txt = temp / "world_benchmark_stdout.txt";
  if (std::system((executableCommand(build_tool) + " " + quote(model) +
                   " --half-extent 0.5 0.5 0.5 > " +
                   quote(temp / "world_benchmark_build.txt"))
                      .c_str()) != 0 ||
      !writeFixtureFiles(scene, model, samples)) {
    return 1;
  }
  const std::string command =
      executableCommand(bench_tool) + " " + quote(scene) +
      " --mode sparse --repeat 2 --warmup 1 --csv " + quote(out_csv) +
      " > " + quote(stdout_txt);
  if (std::system(command.c_str()) != 0 ||
      readFile(stdout_txt).find("avg_total_ms") == std::string::npos ||
      readFile(out_csv).find("sparse") == std::string::npos) {
    std::cerr << "collision world benchmark CLI failed\n";
    return 1;
  }
  std::cout << "collision world benchmark CLI passed\n";
  return 0;
}
