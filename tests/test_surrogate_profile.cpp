#include <adasdf/adasdf.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>

#ifndef ADASDF_CL_TEST_TEMP_DIR
#define ADASDF_CL_TEST_TEMP_DIR ""
#endif

int main() {
  try {
    const adasdf::SurrogateProfileLoadResult defaults =
        adasdf::SurrogateProfile::defaults();
    if (!defaults.loaded || defaults.warnings.empty() ||
        defaults.options.max_max_level < defaults.options.min_max_level) {
      std::cerr << "default profile invalid\n";
      return 1;
    }

    std::filesystem::path temp = ADASDF_CL_TEST_TEMP_DIR;
    if (temp.empty()) {
      temp = std::filesystem::temp_directory_path();
    }
    std::filesystem::create_directories(temp);
    const auto profile_path = temp / "surrogate_profile_test.txt";
    {
      std::ofstream file(profile_path);
      file << "memory_safety_factor=1.5\n";
      file << "max_max_level=6\n";
      file << "unknown_key=hello\n";
      file << "min_rank=bad\n";
    }
    const adasdf::SurrogateProfileLoadResult loaded =
        adasdf::SurrogateProfile::load(profile_path.string());
    if (!loaded.loaded || loaded.options.memory_safety_factor != 1.5 ||
        loaded.options.max_max_level != 6 || loaded.warnings.size() < 2) {
      std::cerr << "profile load did not preserve valid values/warnings\n";
      return 1;
    }

    const adasdf::SurrogateProfileLoadResult missing =
        adasdf::SurrogateProfile::load((temp / "missing_profile.txt").string());
    if (missing.loaded || missing.warnings.empty()) {
      std::cerr << "missing profile did not fall back with warning\n";
      return 1;
    }
    std::cout << "surrogate profile passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_surrogate_profile failed: " << exc.what() << "\n";
    return 1;
  }
}
