#include <adasdf/adasdf.h>

static_assert(static_cast<int>(adasdf::QueryMode::Balanced) >= 0,
              "QueryMode must be part of the public collision API");

int main() {
  return 0;
}
