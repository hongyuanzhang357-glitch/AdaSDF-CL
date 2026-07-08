#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

inline std::string executableCommand(const std::string& tool) {
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

inline std::string quotePath(const std::filesystem::path& path) {
  return "\"" + path.string() + "\"";
}

inline std::string readText(const std::filesystem::path& path) {
  std::ifstream file(path);
  std::ostringstream out;
  out << file.rdbuf();
  return out.str();
}

inline bool containsText(const std::string& text, const std::string& needle) {
  return text.find(needle) != std::string::npos;
}

inline int runCommand(const std::string& command) {
  return std::system(command.c_str());
}
