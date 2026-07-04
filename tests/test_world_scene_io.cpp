#include <adasdf/adasdf.h>

#include <filesystem>
#include <fstream>
#include <iostream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
  std::filesystem::create_directories(temp);
  const auto scene = temp / "world_scene_io.csv";
  {
    std::ofstream file(scene);
    file << "object_id,name,model_path,samples_path,tx,ty,tz,qw,qx,qy,qz,group,mask,type,enabled\n";
    file << "0,base,,,0,0,0,1,0,0,0,0x1,0xffffffff,static,true\n";
    file << "1,tool,,,1,0,0,1,0,0,0,0x2,0xffffffff,dynamic,false\n";
  }
  adasdf::WorldSceneReadOptions options;
  options.require_models = false;
  const adasdf::WorldSceneReadResult result =
      adasdf::WorldSceneIO::readCSV(scene, options);
  if (!result.success || result.world.objectCount() != 2 ||
      result.world.findObject(1)->enabled) {
    std::cerr << "world scene CSV read failed: " << result.error_message << "\n";
    return 1;
  }
  const auto roundtrip = temp / "world_scene_io_roundtrip.csv";
  std::string error;
  if (!adasdf::WorldSceneIO::writeCSV(roundtrip, result.world, &error) ||
      !std::filesystem::exists(roundtrip)) {
    std::cerr << "world scene CSV write failed: " << error << "\n";
    return 1;
  }

  const std::filesystem::path utf8_dir =
      temp / std::filesystem::u8path("utf8_"
                                    "\xE8\xB7\xAF\xE5\xBE\x84");
  std::filesystem::create_directories(utf8_dir);
  const auto samples = utf8_dir / "samples.csv";
  {
    std::ofstream file(samples);
    file << "id,x,y,z,radius,object_id,link_id,group_id,weight,label\n";
    file << "0,0,0,0,0,0,0,0,1,origin\n";
  }
  const auto utf8_scene = temp / "world_scene_utf8_paths.csv";
  {
    std::ofstream file(utf8_scene);
    file << "object_id,name,model_path,samples_path,tx,ty,tz,qw,qx,qy,qz,group,mask,type,enabled\n";
    file << "2,utf8_sample,," << samples.u8string()
         << ",0,0,0,1,0,0,0,0x1,0xffffffff,dynamic,true\n";
  }
  adasdf::WorldSceneReadOptions utf8_options;
  utf8_options.require_models = false;
  const adasdf::WorldSceneReadResult utf8_result =
      adasdf::WorldSceneIO::readCSV(utf8_scene, utf8_options);
  if (!utf8_result.success || utf8_result.world.objectCount() != 1 ||
      !utf8_result.world.objects().front().has_samples) {
    std::cerr << "world scene UTF-8 path read failed: "
              << utf8_result.error_message << "\n";
    return 1;
  }
  std::cout << "world scene IO passed\n";
  return 0;
}
