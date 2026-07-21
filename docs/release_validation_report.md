# AdaSDF-CL v1.18.0-alpha Release Validation

## Release identity

- Branch: `release/v1.18.0-alpha`
- Base commit: `bf2bfc21626138066e7d5768fe13d9d83ffde802`
- Version: `1.18.0-alpha`
- Positioning: Adaptive SDF Compiler Preview

## Curated scope

The release stages generic adaptive generation, BVH sampling, block/tensor
compression, constrained SDF serialization/query, diagnostics, and formal
tests. It excludes local geometry reports and all learned-advisor,
training/calibration, optimizer, dataset, weight, cache, benchmark-output, and
generated-figure content.

## Validation matrix

| Configuration | Configure | Build | CTest | Result |
| --- | --- | --- | --- | --- |
| CPU Release (`ADASDF_CL_ENABLE_CUDA=OFF`) | PASS | PASS | 286/286 PASS (33.86 s) | PASS |
| CUDA Release (`ADASDF_CL_ENABLE_CUDA=ON`) | PASS | PASS | 286/286 PASS (31.22 s) | PASS |

Both configurations registered exactly `286` tests. The CUDA build used NVCC
12.6.85 and compiled `CudaQueryKernels.cu` plus
`CudaActiveBlockKernels.cu`; CUDA runtime tests passed in the CUDA-enabled
configuration.

## Repository checks

- Forbidden experiment scan: PASS before commit
- `scripts/check_repo_clean.py`: PASS
- Prototype branch/HEAD/dirty-tree preservation: PASS before commit
- Release archive SHA-256: recorded after commit in the release finalization output
- Release commit: assigned after this report is committed
- Annotated tag: created only after the release commit passes final checks
