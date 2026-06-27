#include <adasdf/adasdf.h>

#include <iostream>

int main() {
  using namespace adasdf;

  auto model = SDFBinReader::read("gear.sdfbin");
  CollisionObject object_a(model);
  CollisionObject object_b(model);

  BatchedCollisionRequest batch;
  batch.request.backend = BackendType::CUDA;
  batch.request.query_mode = QueryMode::Fast;
  batch.request.enable_contact = true;
  batch.objects_a.push_back(&object_a);
  batch.objects_b.push_back(&object_b);

  // Interface preview: future implementation should call CUDA kernels only
  // when the backend is available, otherwise report a clear fallback/error.
  BatchedCollisionResult results = collideBatch(batch);
  std::cout << "batch results: " << results.results.size() << "\n";
}
