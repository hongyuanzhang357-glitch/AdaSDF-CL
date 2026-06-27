#include <adasdf/adasdf.h>

int main() {
  using namespace adasdf;

  BuildOptions options;
  options.near_surface_error = 1e-4;
  options.max_octree_level = 7;
  options.max_memory_mb = 512;
  options.enable_compression = true;
  options.backend = BackendType::CPU;

  // Interface preview: this should later call the existing STL reader,
  // OctreeSDF builder, block partitioner, and adaptive compressor.
  auto sdf = AdaptiveSDFBuilder::fromMesh("gear.stl", options);
  SDFBinWriter::write("gear.sdfbin", *sdf);
}
