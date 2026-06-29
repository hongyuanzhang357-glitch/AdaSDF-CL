# Demo Surrogate Recommender

AdaSDF-CL v0.9.0-alpha includes an experimental demo surrogate recommender.

## Boundary

The recommender is:

- experimental demo infrastructure;
- not universal;
- not fully trained;
- not an optimality guarantee;
- not an industrial parameter selector;
- not a reliable recommender for arbitrary STL inputs.

It exists to exercise the public workflow from target accuracy and memory constraints to demo adaptive SDF metadata, collision, and visualization.

## Inputs

```cpp
DemoSurrogateInput input;
input.shape_type = "box";
input.target_near_surface_error = 1.0e-3;
input.memory_limit_mb = 64.0;
input.block_expand_limit_mb = 16.0;
input.top_k = 5;
```

## Outputs

Each `DemoSurrogateCandidate` includes:

- `BuildOptions`;
- predicted p95 error;
- predicted memory;
- confidence;
- `credible_demo_only = true`;
- a warning string containing the demo boundary.

## Implementation

v0.9 uses a deterministic C++ demo recommender, not a copied trained model. The repository includes only lightweight public metadata under `models/surrogate/demo_v0_9/`.

The rule table maps smaller target errors to higher octree levels, block resolution, and rank. Smaller memory limits constrain block count and rank.

## CLI

```bash
adasdf_recommend_demo --shape box --target-error 1e-3 --memory-mb 64 --block-memory-mb 16 --top-k 5
```
