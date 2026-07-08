#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::BackendJsonContract contract;
  contract.schema_id = adasdf::SchemaIds::Benchmark;
  contract.tool_name = "unit";
  contract.warnings.push_back({"W", "quoted \" warning"});
  contract.payload_fields.push_back({"value", "1"});
  const std::string json = adasdf::JsonContractWriter::writeObject(contract);
  if (json.find("\"schema_id\": \"adasdf.benchmark.v1\"") ==
          std::string::npos ||
      json.find("\"warnings\": [{\"code\":\"W\"") == std::string::npos ||
      json.find("\"value\": 1") == std::string::npos) {
    std::cerr << "writer output missing expected fields\n";
    return 1;
  }
  return 0;
}
