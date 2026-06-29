# Limitations

AdaSDF-CL 1.0.1-alpha is a research-preview library layer, not a certified industrial collision stack.

## Demo Surrogate

- Experimental demo recommender only.
- Not universal.
- Not fully trained.
- Not an optimality guarantee.
- Not a reliable parameter selector for arbitrary STL inputs.

## Demo Adaptive Builder

- Uses analytic box SDF queries and demo adaptive metadata.
- Does not implement full STL adaptive octree + block-low-rank construction.
- Demo `.sdfbin` formats are public workflow formats, not the full compressed adaptive format.

## Collision

- Pair collision uses `CpuNarrowPhase`, an approximate SDF-sampling narrow-phase.
- Contact reduction is deterministic but not a manifold optimizer.
- Distance for overlapping AABBs is approximate and sample based.

## CUDA Batch Query

- CUDA is optional and disabled by default.
- CUDA query requires pre-expanded global or block dense SDF data.
- `CUDA + None` is invalid because compressed-direct GPU query is not implemented.
- The current resident CUDA backend supports analytic/demo adaptive box expansions only.
- Full low-rank compressed SDF GPU expansion is not implemented.
- Expanded-grid finite-difference normals are not certified contact normals.
- CUDA-unavailable machines should skip GPU benchmarks/tests rather than fail CPU builds.

## Not Implemented

- CUDA batched pair collision.
- Python bindings.
- FCL ABI compatibility.
- Industrial certified contact solver.
- Full public standalone adaptive STL-to-SDF builder.
