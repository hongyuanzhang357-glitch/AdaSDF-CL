# Adaptive Block SDF vs DenseSDF

AdaSDF-CL currently exposes two public core-free STL-to-SDF builders.

| Builder | Since | Grid | Storage | Compression |
| --- | --- | --- | --- | --- |
| DenseSDF | v1.5 | Uniform global grid | Dense phi values | None |
| AdaptiveBlockSDF | v1.6 | Octree leaves + per-leaf blocks | Dense phi values per block | None |
| Low-rank compressed SDF | Planned v1.7 | Adaptive blocks | Low-rank factors | Planned |

DenseSDF is the simpler baseline and is useful for predictable memory/layout
behavior. AdaptiveBlockSDF refines the domain into octree leaves and stores a
dense block per leaf. v1.6 does not guarantee lower memory than DenseSDF; the
main deliverable is the public adaptive octree/block build and query path.

Use `examples/17_adaptive_vs_dense_sdf_demo.cpp` for a small comparison frame.
