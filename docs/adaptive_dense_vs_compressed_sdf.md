# Dense vs Adaptive vs Compressed SDF

AdaSDF-CL currently exposes three public core-free STL-to-SDF representations.

| Representation | Since | Storage | Strength | Boundary |
| --- | --- | --- | --- | --- |
| DenseSDF | v1.5 | one uniform dense grid | simple baseline | memory grows cubically with resolution |
| AdaptiveBlockSDF | v1.6 | dense phi grid per adaptive block | octree-refined domain | block payloads are not compressed |
| CompressedAdaptiveBlockSDF | v1.7 | matrix-SVD or dense-fallback blocks | lower-rank adaptive payloads with audit metrics | Tucker and GPU-native compressed query are planned |

## Query Support

All three support:

- `adasdf_info`;
- `adasdf_query`;
- `adasdf_collide`;
- `adasdf_expansion_quality`;
- `adasdf_benchmark_batch_query --model`.

Compressed adaptive models query directly on CPU by reconstructing needed block
grid values on demand. CUDA still uses expanded SDF layouts.

## Example

`examples/19_dense_adaptive_compressed_comparison_demo.cpp` builds all three
representations from the project-generated closed cube fixture and compares
memory footprint and sampled phi differences.
