#include <adasdf/adasdf.h>

#include <exception>
#include <iostream>

int main() {
  try {
    const auto key = adasdf::MortonKey::encode3D(17, 5, 29);
    const adasdf::Int3 decoded = adasdf::MortonKey::decode3D(key);
    if (decoded.x != 17 || decoded.y != 5 || decoded.z != 29) {
      std::cerr << "Morton encode/decode mismatch\n";
      return 1;
    }
    if (key != adasdf::MortonKey::encode3D(17, 5, 29)) {
      std::cerr << "Morton key is not deterministic\n";
      return 1;
    }
    std::cout << "morton key passed\n";
    return 0;
  } catch (const std::exception& exc) {
    std::cerr << "test_morton_key failed: " << exc.what() << "\n";
    return 1;
  }
}
