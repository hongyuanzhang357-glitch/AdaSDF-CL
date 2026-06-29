#include <adasdf/adasdf.h>

#include <iostream>
#include <stdexcept>
#include <string>

int main() {
  const adasdf::BlockSelection all = adasdf::BlockSelection::all();
  if (!all.use_all_blocks || adasdf::blockSelectionString(all) != "all") {
    std::cerr << "BlockSelection::all failed\n";
    return 1;
  }

  const adasdf::BlockSelection selected =
      adasdf::BlockSelection::selected({3, 1, 3, 2});
  if (selected.use_all_blocks || selected.block_ids.size() != 3 ||
      selected.block_ids[0] != 1 || selected.block_ids[1] != 2 ||
      selected.block_ids[2] != 3 ||
      adasdf::blockSelectionString(selected) != "1,2,3") {
    std::cerr << "BlockSelection::selected did not sort and unique ids\n";
    return 1;
  }

  adasdf::QueryModeConfig invalid =
      adasdf::QueryModeConfig::cpuBlockExpanded(
          adasdf::BlockSelection::selected({-1}));
  try {
    adasdf::validateQueryModeConfig(invalid);
    std::cerr << "Negative block id was not rejected\n";
    return 1;
  } catch (const std::runtime_error& error) {
    if (std::string(error.what()).find("Block ids must be non-negative") ==
        std::string::npos) {
      std::cerr << "Unexpected negative block id error: " << error.what() << "\n";
      return 1;
    }
  }

  std::cout << "BlockSelection tests passed\n";
  return 0;
}
