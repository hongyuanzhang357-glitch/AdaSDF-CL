#include <adasdf/adasdf.h>

static_assert(sizeof(adasdf::ContactOnlySDFBinHeader) > 0,
              "contact-only sdfbin header should stay visible to tests");

int main() {
  return 0;
}
