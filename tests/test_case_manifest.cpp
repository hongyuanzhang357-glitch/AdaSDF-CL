#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::CaseManifest c;
  c.case_id = "case_manifest";
  c.tool_name = "manifest_tool";
  c.input_path = "input.stl";
  c.output_path = "output.sdfbin";
  c.parameters["rank"] = "8";
  const adasdf::RunManifest run = adasdf::runManifestFromCase(c);
  if (run.case_id != c.case_id || run.tool_name != c.tool_name ||
      run.parameters.at("rank") != "8") {
    std::cerr << "case manifest conversion failed\n";
    return 1;
  }
  std::cout << "case manifest passed\n";
  return 0;
}

