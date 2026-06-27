#pragma once

namespace adasdf {

struct PythonBindingPlan {
  bool expose_model_io = true;
  bool expose_collision_object = true;
  bool expose_collision_query = true;
  bool expose_batched_gpu_query = true;
  bool expose_numpy_buffers = true;
};

}  // namespace adasdf
