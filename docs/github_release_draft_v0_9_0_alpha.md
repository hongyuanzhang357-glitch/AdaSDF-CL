# AdaSDF-CL v0.9.0-alpha

First clone-only surrogate-guided adaptive demo workflow.

## Highlights

- Added experimental demo surrogate recommender.
- Added demo adaptive SDF builder with octree, block, and rank metadata.
- Added demo adaptive `.sdfbin` format `ADASDF_DEMO_ADAPTIVE_SDFBIN_V1`.
- Added `adasdf_recommend_demo`.
- Added `adasdf_build_demo_adaptive`.
- Added `adasdf_collide_boxes_demo`.
- Added SVG collision visualization.
- Added examples and tests for the surrogate-guided adaptive demo path.

## Quick Start

```bash
cmake -S . -B build -DADASDF_CL_BUILD_EXAMPLES=ON -DADASDF_CL_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix install

install/bin/adasdf_recommend_demo --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --top-k 5
install/bin/adasdf_build_demo_adaptive cube_adaptive.sdfbin --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --use-surrogate
install/bin/adasdf_collide_boxes_demo --target-error 1e-3 --memory-mb 64 --offset 0.25 0 0 --max-contacts 8 --view collision.svg
```

## Required Caveats

- Demo recommender only.
- Not universal.
- Not fully trained.
- Not an industrial parameter guarantee.
- Full adaptive STL-to-compressed-SDF builder remains future work.
- CUDA, Python bindings, and FCL ABI compatibility are not implemented.

## Validation

Expected validation:

- CMake configure/build: PASS
- CTest: PASS
- Install validation: PASS
- Alpha validation: PASS
- Demo SVG generated and contains contact markers
- Repo clean check: PASS

The earlier tags `v0.7.0-alpha`, `v0.7.0-alpha.1`, `v0.7.0-alpha.2`, and `v0.8.0-alpha` are retained for traceability and should not be moved.
