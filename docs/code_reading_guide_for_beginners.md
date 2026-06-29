# Code Reading Guide for Beginners

## Start Here

- Public umbrella header: `include/adasdf/adasdf.h`
- Version metadata: `include/adasdf/version.h`
- Demo surrogate: `include/adasdf/generation/DemoSurrogateRecommender.h`
- Demo adaptive builder: `include/adasdf/generation/DemoAdaptiveSDFBuilder.h`
- Demo adaptive model: `include/adasdf/geometry/DemoAdaptiveSDFModel.h`
- Demo adaptive reader/writer: `include/adasdf/io/DemoAdaptiveSDFBin.h`
- SVG writer: `include/adasdf/visualization/CollisionSVGWriter.h`
- CPU batch query: `include/adasdf/query/BatchQuery.h`
- Optional CUDA batch query: `include/adasdf/backend/CudaQueryBackend.h`
- Benchmark point generator: `include/adasdf/benchmark/PointCloudGenerator.h`

## Suggested Order

1. Read the public headers above.
2. Read `src/generation/DemoSurrogateRecommender.cpp` for deterministic recommendation rules.
3. Read `src/generation/DemoAdaptiveSDFBuilder.cpp` for three-layer demo metadata generation.
4. Read `src/io/DemoAdaptiveSDFBin.cpp` for the v0.9 text format.
5. Read `src/query/CpuNarrowPhase.cpp` for approximate collision behavior.
6. Read `src/query/BatchQuery.cpp` for the CPU batch query baseline.
7. Read `src/backend/CudaQueryBackend.cpp` for CUDA-unavailable behavior and dispatch.
8. Read `docs/limitations.md` before interpreting results as guarantees.

## Do Not Assume

- The demo surrogate is universal or fully trained.
- The demo adaptive builder is the full STL-to-SDF builder.
- Pair collision is certified or exact.
- Optional CUDA batch query is a full GPU collision pipeline.
