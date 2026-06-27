#include <adasdf/adasdf.h>

int main() {
  const adasdf::ContactOnlySDFBinHeader header;
  return header.version == 1 ? 0 : 1;
}
