#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  adasdf::BackendJsonContract contract;
  contract.schema_id = adasdf::SchemaIds::Info;
  contract.tool_name = "unit";
  contract.payload_fields.push_back(
      {"model_type", adasdf::JsonContractWriter::quote("DenseSDF")});
  const std::string json = adasdf::JsonContractWriter::writeObject(contract);
  const auto result =
      adasdf::JsonContractValidator::validateCommonFields(
          json,
          adasdf::SchemaIds::Info);
  if (!result.valid) {
    std::cerr << "contract validation failed\n";
    return 1;
  }
  return 0;
}
